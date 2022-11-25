// Copyright 2022, Sophos Limited. All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "common/ApplicationPaths.h"
#include "common/Common.h"
#include "common/MockScanner.h"
#include "common/TestFile.h"
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

class QuarantineManagerRescanTests : public LogInitializedTests
{
public:
    void SetUp() override
    {
        m_mockFileSystem = new StrictMock<MockFileSystem>;
        Tests::replaceFileSystem(std::unique_ptr<StrictMock<MockFileSystem>>(m_mockFileSystem));

        setupFakeSophosThreatDetectorConfig();
        setupFakeSafeStoreConfig();
        addCommonPersistValueExpects();

        m_mockSafeStoreWrapper = new StrictMock<MockISafeStoreWrapper>;
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
            *m_mockFileSystem, writeFile(Plugin::getPluginVarDirPath() + "/persist-safeStoreDbErrorThreshold", "10"));
    }

    StrictMock<MockISafeStoreWrapper>* m_mockSafeStoreWrapper;
    StrictMock<MockFileSystem>* m_mockFileSystem;

    // Common test constants
    inline static const std::string m_dir = "/dir";
    inline static const std::string m_file = "file";
    inline static const std::string m_threatID = "01234567-89ab-cdef-0123-456789abcdef";
    inline static const std::string m_threatName = "threatName";
    inline static const std::string m_SHA256 = "SHA256abcdef";
};

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesForRestoreListDoesNothingWithEmptyArgs)
{
    testing::internal::CaptureStderr();
    QuarantineManagerImpl quarantineManager{ std::unique_ptr<StrictMock<MockISafeStoreWrapper>>(
        m_mockSafeStoreWrapper) };

    std::vector<FdsObjectIdsPair> testFiles;

    quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("No files to Rescan"));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFiles)
{
    QuarantineManagerImpl quarantineManager{ std::unique_ptr<StrictMock<MockISafeStoreWrapper>>(
        m_mockSafeStoreWrapper) };
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };
    auto fd1_response = scan_messages::ScanResponse();
    const std::string threatPath1 = "one";

    TestFile dirtyFile1("dirtyFile1");
    datatypes::AutoFd fd2{ dirtyFile1.open() };
    auto fd2_response = scan_messages::ScanResponse();
    fd2_response.addDetection("two", "THREAT", "sha256");
    const std::string threatPath2 = "two";

    TestFile cleanFile2("cleanFile2");
    datatypes::AutoFd fd3{ cleanFile2.open() };
    auto fd3_response = scan_messages::ScanResponse();
    const std::string threatPath3 = "three";

    TestFile dirtyFile2("dirtyFile2");
    datatypes::AutoFd fd4{ dirtyFile2.open() };
    auto fd4_response = scan_messages::ScanResponse();
    fd4_response.addDetection("four", "THREAT", "sha256");
    const std::string threatPath4 = "four";

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
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg1)).WillOnce(Return(threatPath1));

    void* rawHandle2 = reinterpret_cast<SafeStoreObjectHandle>(2222);
    auto objectHandle2 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle2->getRawHandlePtr() = rawHandle2;
    auto handleAsArg2 = Property(&ObjectHandleHolder::getRawHandle, rawHandle2); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle2));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId2", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg2)).WillOnce(Return(threatPath2));

    void* rawHandle3 = reinterpret_cast<SafeStoreObjectHandle>(3333);
    auto objectHandle3 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle3->getRawHandlePtr() = rawHandle3;
    auto handleAsArg3 = Property(&ObjectHandleHolder::getRawHandle, rawHandle3); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle3));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId3", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg3)).WillOnce(Return(threatPath3));

    void* rawHandle4 = reinterpret_cast<SafeStoreObjectHandle>(4444);
    auto objectHandle4 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle4->getRawHandlePtr() = rawHandle4;
    auto handleAsArg4 = Property(&ObjectHandleHolder::getRawHandle, rawHandle4); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle4));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId4", _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(handleAsArg4)).WillOnce(Return(threatPath4));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).Times(4)
        .WillOnce(Return(ByMove(std::move(objectHandle1))))
        .WillOnce(Return(ByMove(std::move(objectHandle2))))
        .WillOnce(Return(ByMove(std::move(objectHandle3))))
        .WillOnce(Return(ByMove(std::move(objectHandle4))));

    EXPECT_CALL(*scanner, scan(_, _, _, _))
        .Times(4)
        .WillOnce(Return(fd1_response))
        .WillOnce(Return(fd2_response))
        .WillOnce(Return(fd3_response))
        .WillOnce(Return(fd4_response));
    EXPECT_CALL(*scannerFactory, createScanner(true, true)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(Plugin::getScanningClientSocketPath(), 0600, scannerFactory);
    server.start();

    std::vector<std::string> expectedResult{ "objectId1", "objectId3" };
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    server.requestStop();
    server.join();
}


TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSocketFailure)
{
    testing::internal::CaptureStderr();
    QuarantineManagerImpl quarantineManager{ std::unique_ptr<StrictMock<MockISafeStoreWrapper>>(
        m_mockSafeStoreWrapper) };

    TestFile cleanFile1("cleanFile1");
    datatypes::AutoFd fd1{ cleanFile1.open() };

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(std::move(fd1), "objectId1");

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("ERROR Error on rescan request: "));
}


TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSkipsHandleFailure)
{
    testing::internal::CaptureStderr();
    QuarantineManagerImpl quarantineManager{ std::unique_ptr<StrictMock<MockISafeStoreWrapper>>(
        m_mockSafeStoreWrapper) };
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

    auto objectHandle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectHandle("objectId1", _)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle))));

    EXPECT_CALL(*scanner, scan(_, _, _, _)).WillOnce(Return(fd1_response));
    EXPECT_CALL(*scannerFactory, createScanner(true, true)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(Plugin::getScanningClientSocketPath(), 0600, scannerFactory);
    server.start();

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles));
    EXPECT_EQ(expectedResult, result);

    server.requestStop();
    server.join();

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("ERROR Couldn't get object handle for: objectId1, continuing..."));
}
