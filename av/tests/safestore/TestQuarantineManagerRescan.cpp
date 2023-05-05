// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockISafeStoreWrapper.h"
#include "MockSafeStoreResources.h"

#include "../common/MemoryAppender.h"
#include "common/ApplicationPaths.h"
#include "common/Common.h"
#include "common/MockScanner.h"
#include "common/TestFile.h"
#include "datatypes/MockSysCalls.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"
#include "scan_messages/QuarantineResponse.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace safestore::QuarantineManager;
using namespace safestore;
namespace fs = sophos_filesystem;

namespace
{
    class QuarantineManagerRescanTests : public MemoryAppenderUsingTests
    {
    public:
        QuarantineManagerRescanTests() : MemoryAppenderUsingTests("safestore")
        {
        }

        void SetUp() override
        {
            auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
            m_mockFileSystem = mockFileSystem.get(); // keep as a borrowed pointer
            Tests::replaceFileSystem(std::move(mockFileSystem));

            setupFakeSophosThreatDetectorConfig();
            setupFakeSafeStoreConfig();
            addCommonPersistValueExpects();

            m_mockSafeStoreWrapperPtr = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
            m_mockSafeStoreWrapper = m_mockSafeStoreWrapperPtr.get(); // BORROWED pointer
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
            Tests::restoreFileSystem();
        }

        void addCommonPersistValueExpects() const
        {
            EXPECT_CALL(*m_mockFileSystem, exists(Plugin::getPluginVarDirPath() + "/persist-safeStoreDbErrorThreshold"))
                .WillOnce(Return(false));
            EXPECT_CALL(
                *m_mockFileSystem,
                writeFile(Plugin::getPluginVarDirPath() + "/persist-safeStoreDbErrorThreshold", "10"));
        }

        QuarantineManagerImpl createQuarantineManager()
        {
            return { std::move(m_mockSafeStoreWrapperPtr), m_mockSysCallWrapper, mockSafeStoreResources_ };
        }

        MockISafeStoreWrapper* m_mockSafeStoreWrapper = nullptr; // BORROWED pointer
        std::unique_ptr<MockISafeStoreWrapper> m_mockSafeStoreWrapperPtr;
        MockFileSystem* m_mockFileSystem = nullptr; // BORROWED pointer
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper =
            std::make_shared<StrictMock<MockSystemCallWrapper>>();
        MockSafeStoreResources mockSafeStoreResources_;
    };

    using safestore::SafeStoreWrapper::ISafeStoreWrapper;
} // namespace

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesForRestoreListDoesNothingWithEmptyArgs)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto quarantineManager = createQuarantineManager();

    std::vector<FdsObjectIdsPair> testFiles;

    quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));

    EXPECT_TRUE(appenderContains("No files to Rescan"));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFiles)
{
    auto quarantineManager = createQuarantineManager();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    const std::string threatPath = "/threat/path";

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };
    auto fd1_response = scan_messages::ScanResponse();
    const std::string threatName1 = "one";

    TestFile dirtyFile1("dirtyFile1");
    datatypes::AutoFd fd2{ dirtyFile1.open() };
    auto fd2_response = scan_messages::ScanResponse();
    fd2_response.addDetection("two", "THREAT", "sha256");
    const std::string threatName2 = "two";

    TestFile cleanFile2("cleanFile2");
    datatypes::AutoFd fd3{ cleanFile2.open() };
    auto fd3_response = scan_messages::ScanResponse();
    const std::string threatName3 = "three";

    TestFile dirtyFile2("dirtyFile2");
    datatypes::AutoFd fd4{ dirtyFile2.open() };
    auto fd4_response = scan_messages::ScanResponse();
    fd4_response.addDetection("four", "THREAT", "sha256");
    const std::string threatName4 = "four";

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(std::move(fd1), "objectId1");
    testFiles.emplace_back(std::move(fd2), "objectId2");
    testFiles.emplace_back(std::move(fd3), "objectId3");
    testFiles.emplace_back(std::move(fd4), "objectId4");

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle1 = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle1 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle1->getRawHandlePtr() = rawHandle1;
    auto handleAsArg1 = Property(&ObjectHandleHolder::getRawHandle, rawHandle1); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle1));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId1", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg1)).WillOnce(Return(threatPath));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg1)).WillRepeatedly(Return(threatName1));

    void* rawHandle2 = reinterpret_cast<SafeStoreObjectHandle>(2222);
    auto objectHandle2 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle2->getRawHandlePtr() = rawHandle2;
    auto handleAsArg2 = Property(&ObjectHandleHolder::getRawHandle, rawHandle2); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle2));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId2", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg2)).WillOnce(Return(threatPath));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg2)).WillRepeatedly(Return(threatName2));

    void* rawHandle3 = reinterpret_cast<SafeStoreObjectHandle>(3333);
    auto objectHandle3 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle3->getRawHandlePtr() = rawHandle3;
    auto handleAsArg3 = Property(&ObjectHandleHolder::getRawHandle, rawHandle3); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle3));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId3", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg3)).WillOnce(Return(threatPath));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg3)).WillRepeatedly(Return(threatName3));

    void* rawHandle4 = reinterpret_cast<SafeStoreObjectHandle>(4444);
    auto objectHandle4 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle4->getRawHandlePtr() = rawHandle4;
    auto handleAsArg4 = Property(&ObjectHandleHolder::getRawHandle, rawHandle4); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle4));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId4", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg4)).WillOnce(Return(threatPath));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg4)).WillRepeatedly(Return(threatName4));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .Times(4)
        .WillOnce(Return(ByMove(std::move(objectHandle1))))
        .WillOnce(Return(ByMove(std::move(objectHandle2))))
        .WillOnce(Return(ByMove(std::move(objectHandle3))))
        .WillOnce(Return(ByMove(std::move(objectHandle4))));

    EXPECT_CALL(*scanner, scan(_, _))
        .Times(4)
        .WillOnce(Return(fd1_response))
        .WillOnce(Return(fd2_response))
        .WillOnce(Return(fd3_response))
        .WillOnce(Return(fd4_response));
    EXPECT_CALL(*scannerFactory, detectPUAsEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*scannerFactory, createScanner(true, true, true)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(Plugin::getScanningSocketPath(), 0600, scannerFactory);
    server.start();

    std::vector<std::string> expectedResult{ "objectId1", "objectId3" };
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    server.requestStop();
    server.join();
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSocketFailure)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));
    auto quarantineManager = createQuarantineManager();

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(std::move(fd1), "objectId1");

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    EXPECT_TRUE(appenderContains("[ERROR] Error on rescan request: "));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSkipsHandleFailure)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    const std::string threatPath = "/threat/path";

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };
    auto fd1_response = scan_messages::ScanResponse();
    const std::string threatPath1 = "one";

    TestFile dirtyFile1("dirtyFile1");
    datatypes::AutoFd fd2{ dirtyFile1.open() };
    auto fd2_response = scan_messages::ScanResponse();
    fd2_response.addDetection("two", "THREAT", "sha256");
    const std::string threatName2 = "two";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(std::move(fd1), "objectId1");
    testFiles.emplace_back(std::move(fd2), "objectId2");

    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId1", _)).WillOnce(Return(false));

    EXPECT_CALL(*scanner, scan(_, _)).Times(2).WillOnce(Return(fd1_response)).WillOnce(Return(fd2_response));
    EXPECT_CALL(*scannerFactory, detectPUAsEnabled()).WillOnce(Return(true));
    EXPECT_CALL(*scannerFactory, createScanner(true, true, true)).WillOnce(Return(ByMove(std::move(scanner))));

    void* rawHandle2 = reinterpret_cast<SafeStoreObjectHandle>(2222);
    auto objectHandle2 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle2->getRawHandlePtr() = rawHandle2;
    auto handleAsArg2 = Property(&ObjectHandleHolder::getRawHandle, rawHandle2); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle2));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId2", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg2)).WillOnce(Return(threatPath));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg2)).WillRepeatedly(Return(threatName2));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .Times(2)
        .WillOnce(Return(ByMove(std::move(objectHandle))))
        .WillOnce(Return(ByMove(std::move(objectHandle2))));

    unixsocket::ScanningServerSocket server(Plugin::getScanningSocketPath(), 0600, scannerFactory);
    server.start();

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("[ERROR] Couldn't get object handle for: objectId1, continuing..."));
    EXPECT_TRUE(appenderContains("Rescan found quarantined file still a threat: /threat/path/two"));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesHandlesNameAndLocationFailure)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };
    auto fd1_response = scan_messages::ScanResponse();
    const std::string threatPath1 = "one";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(std::move(fd1), "objectId1");

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId1", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillRepeatedly(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillRepeatedly(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));

    EXPECT_CALL(*scanner, scan(_, _)).WillOnce(Return(fd1_response));
    EXPECT_CALL(*scannerFactory, createScanner(true, true, true)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(Plugin::getScanningSocketPath(), 0600, scannerFactory);
    server.start();

    std::vector<std::string> expectedResult{ "objectId1" };
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    server.tryStop();
    server.join();

    EXPECT_TRUE(appenderContains("[WARN] Couldn't get object name for: objectId1."));
    EXPECT_TRUE(appenderContains("[WARN] Couldn't get path for: objectId1: Couldn't get object name"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileDoesNothingIfCannotGetObjectHandle)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(false));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).has_value());

    EXPECT_TRUE(appenderContains("[ERROR] Couldn't get object handle for: objectId"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyOnMissingNameAndLocation)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string threatId = "threatId";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return(""));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).has_value());

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore objectId, reason: Couldn't get object name"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyOnMissingLocation)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string threatId = "threatId";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return("onbjectName"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillOnce(Return(""));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).has_value());

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore objectId, reason: Couldn't get object location"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileDoesNothingIfCannotGetCorrelationId)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string objectName = "testName";
    std::string objectLocation = "/path/to/location";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle(objectId, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return(objectName));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillOnce(Return(objectLocation));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(handleAsArg, "correlationId")).WillOnce(Return(""));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).has_value());

    EXPECT_TRUE(appenderContains(
        "[ERROR] Unable to restore /path/to/location/testName, couldn't get correlation ID: Failed to get SafeStore object custom string 'correlationId'"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyIfRestoreFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string objectName = "testName";
    std::string objectLocation = "/path/to/location";
    std::string correlationId = "01234567-89ab-cdef-0123-456789abcdef";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return(objectName));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillOnce(Return(objectLocation));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(handleAsArg, "correlationId"))
        .WillOnce(Return(correlationId));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectById(objectId)).WillOnce(Return(false));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).value().wasSuccessful);

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore clean file: /path/to/location/testName"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyIfDeleteFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string objectName = "testName";
    std::string objectLocation = "/path/to/location";
    std::string correlationId = "01234567-89ab-cdef-0123-456789abcdef";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return(objectName));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillOnce(Return(objectLocation));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(handleAsArg, "correlationId"))
        .WillOnce(Return(correlationId));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectById(objectId)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(objectId)).WillOnce(Return(false));

    EXPECT_FALSE(quarantineManager.restoreFile(objectId).value().wasSuccessful);

    EXPECT_TRUE(appenderContains("[INFO] Restored file to disk: /path/to/location/testName"));
    EXPECT_TRUE(appenderContains(
        "[WARN] File was restored to disk, but unable to remove it from SafeStore database: /path/to/location/testName"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsReportOnSuccess)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto quarantineManager = createQuarantineManager();

    std::string objectId = "objectId";
    std::string objectName = "testName";
    std::string objectLocation = "/path/to/location";
    std::string correlationId = "01234567-89ab-cdef-0123-456789abcdef";

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle->getRawHandlePtr() = rawHandle;
    auto handleAsArg = Property(&ObjectHandleHolder::getRawHandle, rawHandle); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(handleAsArg)).WillOnce(Return(objectName));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg)).WillOnce(Return(objectLocation));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(handleAsArg, "correlationId"))
        .WillOnce(Return(correlationId));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectById(objectId)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(objectId)).WillOnce(Return(true));

    auto result = quarantineManager.restoreFile(objectId);
    EXPECT_TRUE(result.value().wasSuccessful);
    EXPECT_EQ(result.value().path, "/path/to/location/testName");
    EXPECT_EQ(result.value().correlationId, correlationId);

    EXPECT_TRUE(appenderContains("[INFO] Restored file to disk: /path/to/location/testName"));
    EXPECT_TRUE(appenderContains("[DEBUG] ObjectId successfully deleted from database: objectId"));
}

namespace
{
    class QuarantineManagerRescanDefaultMockTests : public MemoryAppenderUsingTests
    {
    protected:
        QuarantineManagerRescanDefaultMockTests() :
            MemoryAppenderUsingTests("safestore"), memoryAppenderHolder_{ *this }
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("SOPHOS_INSTALL", "/tmp");
            appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
        }

        QuarantineManagerImpl createQuarantineManager()
        {
            return { std::move(mockSafeStoreWrapper_), mockSysCallWrapper_, mockSafeStoreResources_ };
        }

        void defineObjectAccessors(
            int id,
            const std::string& threatType,
            const std::string& threatName,
            const std::string& name,
            const std::string& location,
            const std::string& sha256,
            const std::string& objectId)
        {
            void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(id);
            auto property = Property(&ObjectHandleHolder::getRawHandle, rawHandle);

            ON_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(property, "threatType"))
                .WillByDefault(Return(threatType));
            ON_CALL(*mockSafeStoreWrapper_, getObjectThreatName(property)).WillByDefault(Return(threatName));
            ON_CALL(*mockSafeStoreWrapper_, getObjectName(property)).WillByDefault(Return(name));
            ON_CALL(*mockSafeStoreWrapper_, getObjectLocation(property)).WillByDefault(Return(location));
            ON_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(property, "SHA256"))
                .WillByDefault(Return(sha256));
            ON_CALL(*mockSafeStoreWrapper_, getObjectId(property)).WillByDefault(Return(objectId));
        }

        ObjectHandleHolder createObjectHandle(int id)
        {
            ObjectHandleHolder objectHandle{ mockGetIdMethods_, mockReleaseMethods_ };
            *objectHandle.getRawHandlePtr() = reinterpret_cast<SafeStoreObjectHandle>(id);
            return objectHandle;
        }

        UsingMemoryAppender memoryAppenderHolder_;
        std::unique_ptr<MockISafeStoreWrapper> mockSafeStoreWrapper_ = std::make_unique<MockISafeStoreWrapper>();
        std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();
        std::shared_ptr<MockISafeStoreGetIdMethods> mockGetIdMethods_ = std::make_shared<MockISafeStoreGetIdMethods>();
        std::shared_ptr<MockISafeStoreReleaseMethods> mockReleaseMethods_ =
            std::make_shared<MockISafeStoreReleaseMethods>();
        std::shared_ptr<MockSystemCallWrapper> mockSysCallWrapper_ = std::make_shared<MockSystemCallWrapper>();
        MockSafeStoreResources mockSafeStoreResources_;
    };
} // namespace

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanReceivesClean)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "threatName", "name", "/location", "SHA256", "objectId");

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillRepeatedly(InvokeWithoutArgs(
            []()
            {
                auto wrapper = std::make_unique<unixsocket::MockMetadataRescanClientSocket>();
                EXPECT_CALL(*wrapper, rescan).WillRepeatedly(Return(scan_messages::MetadataRescanResponse::ok));
                return wrapper;
            }));

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location/name' and object ID 'objectId'"));
    EXPECT_TRUE(appenderContains(
        "DEBUG - Metadata rescan for '/location/name' found it to no longer have the saved detection"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanReceivesThreat)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "threatName", "name", "/location", "SHA256", "objectId");

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillRepeatedly(InvokeWithoutArgs(
            []()
            {
                auto wrapper = std::make_unique<unixsocket::MockMetadataRescanClientSocket>();
                EXPECT_CALL(*wrapper, rescan)
                    .WillRepeatedly(Return(scan_messages::MetadataRescanResponse::threatPresent));
                return wrapper;
            }));

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location/name' and object ID 'objectId'"));
    EXPECT_TRUE(appenderContains("DEBUG - Metadata rescan for '/location/name' found it to still be a threat"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanReceivesMultipleResults)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                searchResults.push_back(createObjectHandle(2222));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType_1", "threatName_1", "name_1", "/location_1", "SHA256_1", "objectId_1");
    defineObjectAccessors(2222, "threatType_2", "threatName_2", "name_2", "/location_2", "SHA256_2", "objectId_2");

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillRepeatedly(InvokeWithoutArgs(
            []()
            {
                auto wrapper = std::make_unique<unixsocket::MockMetadataRescanClientSocket>();
                EXPECT_CALL(*wrapper, rescan)
                    .WillOnce(Return(scan_messages::MetadataRescanResponse::threatPresent))
                    .WillOnce(Return(scan_messages::MetadataRescanResponse::ok));
                return wrapper;
            }));

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location_1/name_1' and object ID 'objectId_1'"));
    EXPECT_TRUE(appenderContains("DEBUG - Metadata rescan for '/location_1/name_1' found it to still be a threat"));
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location_2/name_2' and object ID 'objectId_2'"));
    EXPECT_TRUE(appenderContains(
        "DEBUG - Metadata rescan for '/location_2/name_2' found it to no longer have the saved detection"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanFailsIfThreatNameIsMissing)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "", "name", "/location", "SHA256", "objectId");

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("Failed to create metadata rescan request: Failed to get object threat name"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanFailsIfNameIsMissing)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "threatName", "", "/location", "SHA256", "objectId");

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("Failed to create metadata rescan request: Couldn't get object name"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanFailsIfLocationIsMissing)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "threatName", "name", "", "SHA256", "objectId");

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("Failed to create metadata rescan request: Couldn't get object location"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanFailsIfSha256IsMissing)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "threatType", "threatName", "name", "/location", "", "objectId");

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("Failed to create metadata rescan request: Failed to get object SHA256"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanSucceedsIfThreatTypeIsMissing)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                return searchResults;
            }));
    defineObjectAccessors(1111, "", "threatName", "name", "/location", "SHA256", "objectId");

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillRepeatedly(InvokeWithoutArgs(
            []()
            {
                auto wrapper = std::make_unique<unixsocket::MockMetadataRescanClientSocket>();
                EXPECT_CALL(*wrapper, rescan(Field(&scan_messages::MetadataRescan::threatType, "virus")))
                    .WillRepeatedly(Return(scan_messages::MetadataRescanResponse::ok));
                return wrapper;
            }));

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location/name' and object ID 'objectId'"));
    EXPECT_TRUE(appenderContains(
        "DEBUG - Metadata rescan for '/location/name' found it to no longer have the saved detection"));
}

TEST_F(QuarantineManagerRescanDefaultMockTests, MetadataRescanContinuesIfOneFails)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    ON_CALL(*mockSafeStoreWrapper_, find)
        .WillByDefault(InvokeWithoutArgs(
            [this]()
            {
                std::vector<ObjectHandleHolder> searchResults;
                searchResults.push_back(createObjectHandle(1111));
                searchResults.push_back(createObjectHandle(2222));
                return searchResults;
            }));
    defineObjectAccessors(1111, "", "", "", "", "", "");
    defineObjectAccessors(2222, "threatType_2", "threatName_2", "name_2", "/location_2", "SHA256_2", "objectId_2");

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillRepeatedly(InvokeWithoutArgs(
            []()
            {
                auto wrapper = std::make_unique<unixsocket::MockMetadataRescanClientSocket>();
                EXPECT_CALL(*wrapper, rescan).WillRepeatedly(Return(scan_messages::MetadataRescanResponse::ok));
                return wrapper;
            }));

    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("Failed to create metadata rescan request:"));
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file with original path '/location_2/name_2' and object ID 'objectId_2'"));
    EXPECT_TRUE(appenderContains(
        "DEBUG - Metadata rescan for '/location_2/name_2' found it to no longer have the saved detection"));
}
