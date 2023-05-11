// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockISafeStoreWrapper.h"
#include "MockSafeStoreResources.h"

#include "../common/MemoryAppender.h"
#include "common/ApplicationPaths.h"
#include "common/TestFile.h"
#include "datatypes/MockSysCalls.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"
#include "scan_messages/QuarantineResponse.h"
#include "tests/unixsocket/MockIScanningClientSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace safestore::QuarantineManager;
using namespace safestore;

MATCHER_P(IsRescanAndHasFd, fd, "")
{
    return arg->getFd() == fd && arg->getPath() == "orgpath" &&
           arg->getScanType() == scan_messages::E_SCAN_TYPE_SAFESTORE_RESCAN && arg->getDetectPUAs() == true;
}

MATCHER_P(HasRawPointer, id, "")
{
    return arg.getRawHandle() == reinterpret_cast<SafeStoreObjectHandle>(id);
}

namespace
{
    class QuarantineManagerRescanTests : public MemoryAppenderUsingTests
    {
    public:
        QuarantineManagerRescanTests() : MemoryAppenderUsingTests("safestore"), usingMemoryAppender_{ *this }
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("SOPHOS_INSTALL", "/tmp");
            appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
            unpackLocation_ = Common::FileSystem::join(Plugin::getPluginVarDirPath(), "tempUnpack");

            ON_CALL(*mockSafeStoreWrapper_, createObjectHandleHolder)
                .WillByDefault(Invoke(
                    [this]() { return std::make_unique<ObjectHandleHolder>(mockGetIdMethods_, mockReleaseMethods_); }));
        }

        QuarantineManagerImpl createQuarantineManager()
        {
            return { std::move(mockSafeStoreWrapper_), mockSysCallWrapper_, mockSafeStoreResources_ };
        }

        struct SafeStoreObjectMetadata
        {
            std::string threatName{ "threatName" };
            std::string name{ "name" };
            std::string location{ "/location" };
            std::string sha256{ "SHA256" };
            std::string threats{ R"([{"type":"threatType","name":"threatName","sha256":"threatSHA256"}])" };
            std::string correlationId{ "fedcba98-7654-3210-fedc-ba9876543210" };
        };

        struct SafeStoreObjectDefinition
        {
            int rawPointer;
            std::string objectId;
            SafeStoreObjectMetadata metadata;
        };

        void defineSafeStoreObject(int id, const std::string& objectId, SafeStoreObjectMetadata metadata) const
        {
            auto matcher = HasRawPointer(id);

            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(matcher, "SHA256"))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.sha256));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(matcher, "correlationId"))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.correlationId));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(matcher, "threats"))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.threats));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectThreatName(matcher))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.threatName));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectName(matcher))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.name));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(matcher))
                .Times(AnyNumber())
                .WillRepeatedly(Return(metadata.location));
            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectId(matcher))
                .Times(AnyNumber())
                .WillRepeatedly(Return(objectId));
            EXPECT_CALL(*mockSafeStoreWrapper_, setObjectCustomDataString(matcher, "SHA256", _))
                .Times(AnyNumber())
                .WillRepeatedly(Return(true));
            EXPECT_CALL(*mockSafeStoreWrapper_, setObjectCustomDataString(matcher, "threats", _))
                .Times(AnyNumber())
                .WillRepeatedly(Return(true));
            EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById(objectId))
                .Times(AnyNumber())
                .WillRepeatedly(Return(true));
            EXPECT_CALL(*mockSafeStoreWrapper_, deleteObjectById(objectId))
                .Times(AnyNumber())
                .WillRepeatedly(Return(true));
            EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation(objectId, unpackLocation_))
                .Times(AnyNumber())
                .WillOnce(Return(true));

            EXPECT_CALL(*mockSafeStoreWrapper_, getObjectHandle(objectId, _))
                .Times(AnyNumber())
                .WillRepeatedly(Invoke(
                    [id](const ObjectId&, const std::shared_ptr<ObjectHandleHolder>& objectHandle)
                    {
                        *objectHandle->getRawHandlePtr() = reinterpret_cast<SafeStoreObjectHandle>(id);
                        return true;
                    }));
        }

        void defineSafeStoreObjects(const std::vector<SafeStoreObjectDefinition>& definitions)
        {
            for (const auto& definition : definitions)
            {
                defineSafeStoreObject(definition.rawPointer, definition.objectId, definition.metadata);
            }

            ON_CALL(*mockSafeStoreWrapper_, find)
                .WillByDefault(InvokeWithoutArgs(
                    [this, definitions]()
                    {
                        std::vector<ObjectHandleHolder> searchResults;
                        for (const auto& definition : definitions)
                        {
                            searchResults.push_back(createObjectHandle(definition.rawPointer));
                        }
                        return searchResults;
                    }));
        }

        ObjectHandleHolder createObjectHandle(int id)
        {
            ObjectHandleHolder objectHandle{ mockGetIdMethods_, mockReleaseMethods_ };
            *objectHandle.getRawHandlePtr() = reinterpret_cast<SafeStoreObjectHandle>(id);
            return objectHandle;
        }

        // To be used when a real fd is needed, as AutoFd can't be mocked
        static int createRealFd()
        {
            return ::open("/dev/null", O_RDONLY);
        }

        void MoveFileSystemMocks()
        {
            scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));
            scopedReplaceFilePermissions_.replace(std::move(mockFilePermissions_));
        }

        UsingMemoryAppender usingMemoryAppender_;
        std::unique_ptr<MockISafeStoreWrapper> mockSafeStoreWrapper_{ std::make_unique<MockISafeStoreWrapper>() };
        std::unique_ptr<MockFileSystem> mockFileSystem_{ std::make_unique<MockFileSystem>() };
        std::unique_ptr<MockFilePermissions> mockFilePermissions_{ std::make_unique<MockFilePermissions>() };
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem_;
        Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions_;
        std::shared_ptr<MockSystemCallWrapper> mockSysCallWrapper_ = std::make_shared<MockSystemCallWrapper>();
        MockSafeStoreResources mockSafeStoreResources_;
        std::unique_ptr<unixsocket::MockMetadataRescanClientSocket> mockMetadataRescanClientSocket_{
            std::make_unique<unixsocket::MockMetadataRescanClientSocket>()
        };
        std::unique_ptr<MockIScanningClientSocket> mockScanningClientSocket_{
            std::make_unique<MockIScanningClientSocket>()
        };
        std::shared_ptr<MockISafeStoreGetIdMethods> mockGetIdMethods_{ std::make_shared<MockISafeStoreGetIdMethods>() };
        std::shared_ptr<MockISafeStoreReleaseMethods> mockReleaseMethods_{
            std::make_shared<MockISafeStoreReleaseMethods>()
        };
        std::string unpackLocation_;

        const std::string threatType_{ "threatType" };
        const std::string threatName_{ "threatName" };
        const std::string name_{ "name" };
        const std::string location_{ "/location" };
        const std::string sha256_{ "SHA256" };
        const std::string threatSha256_{ "threatSHA256" };
        const std::string correlationId_{ "fedcba98-7654-3210-fedc-ba9876543210" };
    };
} // namespace

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesForRestoreListDoesNothingWithEmptyArgs)
{
    // No objects should be extracted
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    auto quarantineManager = createQuarantineManager();

    std::vector<FdsObjectIdsPair> testFiles;

    quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles), "");

    EXPECT_TRUE(appenderContains("No files to Rescan"));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFiles)
{
    // Define SafeStore objects
    defineSafeStoreObject(1111, "objectId1", {});
    defineSafeStoreObject(2222, "objectId2", {});
    defineSafeStoreObject(3333, "objectId3", {});
    defineSafeStoreObject(4444, "objectId4", {});

    // Create distinct fds, used later to verify all files are scanned
    auto fd1{ createRealFd() };
    auto fd2{ createRealFd() };
    auto fd3{ createRealFd() };
    auto fd4{ createRealFd() };

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(datatypes::AutoFd{ fd1 }, "objectId1");
    testFiles.emplace_back(datatypes::AutoFd{ fd2 }, "objectId2");
    testFiles.emplace_back(datatypes::AutoFd{ fd3 }, "objectId3");
    testFiles.emplace_back(datatypes::AutoFd{ fd4 }, "objectId4");

    // Mock responses from the scanning server
    // 1 and 3 are clean, 2 and 4 have threats
    {
        InSequence seq;
        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd1))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd2))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse)
            .WillOnce(Invoke(
                [](scan_messages::ScanResponse& response)
                {
                    response.addDetection("two", "virus", "THREAT", "sha256");
                    return true;
                }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd3))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd4))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse)
            .WillOnce(Invoke(
                [](scan_messages::ScanResponse& response)
                {
                    response.addDetection("four", "virus", "THREAT", "sha256");
                    return true;
                }));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    std::vector<std::string> expectedResult{ "objectId1", "objectId3" };
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles), "orgpath");
    EXPECT_EQ(expectedResult, result);
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSocketFailure)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(datatypes::AutoFd{}, "objectId1");

    EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(false));
    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    // No objects should be extracted
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles), "orgpath");
    EXPECT_EQ(expectedResult, result);

    EXPECT_TRUE(appenderContains("[ERROR] Error on rescan request: "));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesSkipsHandleFailure)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId1", { .name = "one" });
    defineSafeStoreObject(2222, "objectId2", { .name = "two" });
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectHandle("objectId1", _)).WillRepeatedly(Return(false));

    auto fd1{ createRealFd() };
    auto fd2{ createRealFd() };

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(datatypes::AutoFd{ fd1 }, "objectId1");
    testFiles.emplace_back(datatypes::AutoFd{ fd2 }, "objectId2");

    // Mock responses from the scanning server
    {
        InSequence seq;
        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd1))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd2))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse)
            .WillOnce(Invoke(
                [](scan_messages::ScanResponse& response)
                {
                    response.addDetection("two", "virus", "THREAT", "sha256");
                    return true;
                }));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    // Object 1 should not be extracted
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById("objectId1")).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation("objectId1", _)).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    std::vector<std::string> expectedResult{};
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles), "orgpath");
    EXPECT_EQ(expectedResult, result);

    EXPECT_TRUE(appenderContains("[ERROR] Couldn't get object handle for: objectId1, continuing..."));
    EXPECT_TRUE(appenderContains("Rescan found quarantined file still a threat: /location/two"));
}

TEST_F(QuarantineManagerRescanTests, scanExtractedFilesHandlesNameAndLocationFailure)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId1", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectName(HasRawPointer(1111))).WillRepeatedly(Return(""));
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(HasRawPointer(1111))).WillRepeatedly(Return(""));

    auto fd1{ createRealFd() };

    std::vector<FdsObjectIdsPair> testFiles;
    testFiles.emplace_back(datatypes::AutoFd{ fd1 }, "objectId1");

    {
        InSequence seq;
        EXPECT_CALL(*mockScanningClientSocket_, sendRequest(IsRescanAndHasFd(fd1))).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    std::vector<std::string> expectedResult{ "objectId1" };
    auto result = quarantineManager.scanExtractedFilesForRestoreList(std::move(testFiles), "orgpath");
    EXPECT_EQ(expectedResult, result);

    EXPECT_TRUE(appenderContains("[WARN] Couldn't get path for 'objectId1': Couldn't get object name"));
    EXPECT_TRUE(appenderContains("[DEBUG] Rescan found quarantined file no longer a threat: <unknown path>"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileDoesNothingIfCannotGetObjectHandle)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectHandle("objectId", _)).WillOnce(Return(false));

    // No objects should be restored
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").has_value());
    EXPECT_TRUE(appenderContains("[ERROR] Couldn't get object handle for: objectId"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyOnMissingNameAndLocation)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectName(HasRawPointer(1111))).WillRepeatedly(Return(""));
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(HasRawPointer(1111))).WillRepeatedly(Return(""));

    // No objects should be restored
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").has_value());

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore objectId, reason: Couldn't get object name"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyOnMissingLocation)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(HasRawPointer(1111))).WillOnce(Return(""));

    // No objects should be restored
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").has_value());

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore objectId, reason: Couldn't get object location"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileDoesNothingIfCannotGetCorrelationId)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(HasRawPointer(1111), "correlationId"))
        .WillOnce(Return(""));

    // No objects should be restored
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById).Times(0);
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation).Times(0);

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").has_value());

    EXPECT_TRUE(appenderContains(
        "[ERROR] Unable to restore /location/name, couldn't get correlation ID: Failed to get SafeStore object custom string 'correlationId'"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyIfRestoreFails)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById("objectId")).WillOnce(Return(false));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").value().wasSuccessful);

    EXPECT_TRUE(appenderContains("[ERROR] Unable to restore clean file: /location/name"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsEarlyIfDeleteFails)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObject(1111, "objectId", {});
    EXPECT_CALL(*mockSafeStoreWrapper_, deleteObjectById("objectId")).WillOnce(Return(false));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    EXPECT_FALSE(quarantineManager.restoreFile("objectId").value().wasSuccessful);

    EXPECT_TRUE(appenderContains("[INFO] Restored file to disk: /location/name"));
    EXPECT_TRUE(appenderContains(
        "[WARN] File was restored to disk, but unable to remove it from SafeStore database: /location/name"));
}

TEST_F(QuarantineManagerRescanTests, restoreFileReturnsReportOnSuccess)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    auto result = quarantineManager.restoreFile("objectId");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value().wasSuccessful);
    EXPECT_EQ(result.value().path, "/location/name");
    EXPECT_EQ(result.value().correlationId, correlationId_);

    EXPECT_TRUE(appenderContains("[INFO] Restored file to disk: /location/name"));
    EXPECT_TRUE(appenderContains("[DEBUG] ObjectId successfully deleted from database: objectId"));
}

TEST_F(QuarantineManagerRescanTests, RescanDoesMetadataRescanAndReceivesUndetectedSoDoesFullRescan)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        EXPECT_CALL(*mockMetadataRescanClientSocket_, rescan)
            .WillOnce(Return(scan_messages::MetadataRescanResponse::undetected));

        // Does full rescan
        EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation("objectId", unpackLocation_))
            .WillOnce(Return(true));
        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillOnce(Return(ByMove(std::move(mockMetadataRescanClientSocket_))));
    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("DEBUG - Received metadata rescan response: 'undetected'"));
    EXPECT_TRUE(appenderContains(
        "INFO - Performing full rescan of quarantined file (original path '/location/name', object ID 'objectId')"));
}

TEST_F(
    QuarantineManagerRescanTests,
    RescanDoesMetadataRescanWithCorrectParametersAndReceivesUndetectedSoDoesFullRescanWhichReportsNoDetectionsSoRestoresFile)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        // Metadata rescan attempted with expected parameters
        EXPECT_CALL(*mockMetadataRescanClientSocket_, rescan)
            .WillOnce(Invoke(
                [](const scan_messages::MetadataRescan& metadataRescan)
                {
                    EXPECT_EQ(metadataRescan.threat.type, "threatType");
                    EXPECT_EQ(metadataRescan.threat.name, "threatName");
                    EXPECT_EQ(metadataRescan.threat.sha256, "threatSHA256");
                    EXPECT_EQ(metadataRescan.filePath, "/location/name");
                    EXPECT_EQ(metadataRescan.sha256, "SHA256");
                    return scan_messages::MetadataRescanResponse::undetected;
                }));

        // Does full rescan
        EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectByIdToLocation("objectId", unpackLocation_))
            .WillOnce(Return(true));
        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));

        // Tries to restore the file
        EXPECT_CALL(*mockSafeStoreWrapper_, restoreObjectById("objectId")).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillOnce(Return(ByMove(std::move(mockMetadataRescanClientSocket_))));
    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file (original path '/location/name', object ID 'objectId')"));
    EXPECT_TRUE(appenderContains("DEBUG - Received metadata rescan response: 'undetected'"));
    EXPECT_TRUE(appenderContains(
        "INFO - Performing full rescan of quarantined file (original path '/location/name', object ID 'objectId')"));
}

TEST_F(QuarantineManagerRescanTests, RescanMetadataRescanFailsIfThreatsIsMissingSoDoesFullRescan)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(HasRawPointer(1111), "threats"))
            .WillRepeatedly(Return(""));

        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "Failed to perform metadata rescan: Couldn't get object custom data 'threats', continuing with full rescan"));
}

TEST_F(QuarantineManagerRescanTests, RescanMetadataRescanFailsIfNameIsMissingSoDoesFullRescan)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        EXPECT_CALL(*mockSafeStoreWrapper_, getObjectName(HasRawPointer(1111))).WillRepeatedly(Return(""));

        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(
        appenderContains("Failed to perform metadata rescan: Couldn't get object name, continuing with full rescan"));
}

TEST_F(QuarantineManagerRescanTests, RescanMetadataRescanFailsIfLocationIsMissingSoDoesFullRescan)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(HasRawPointer(1111))).WillRepeatedly(Return(""));

        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "Failed to perform metadata rescan: Couldn't get object location, continuing with full rescan"));
}

TEST_F(QuarantineManagerRescanTests, RescanMetadataRescanFailsIfSha256IsMissingSoDoesFullRescan)
{
    defineSafeStoreObjects({ { 1111, "objectId", {} } });

    {
        InSequence seq;

        EXPECT_CALL(*mockSafeStoreWrapper_, getObjectCustomDataString(HasRawPointer(1111), "SHA256"))
            .WillRepeatedly(Return(""));

        ON_CALL(*mockFileSystem_, listAllFilesInDirectoryTree(unpackLocation_))
            .WillByDefault(Return(std::vector<std::string>{ "file" }));

        EXPECT_CALL(*mockScanningClientSocket_, sendRequest).WillOnce(Return(true));
        EXPECT_CALL(*mockScanningClientSocket_, receiveResponse).WillOnce(Return(true));
    }

    EXPECT_CALL(mockSafeStoreResources_, CreateScanningClientSocket)
        .WillOnce(Return(ByMove(std::move(mockScanningClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains(
        "Failed to perform metadata rescan: Couldn't get object custom data 'SHA256', continuing with full rescan"));
}

TEST_F(QuarantineManagerRescanTests, MetadataRescanContinuesIfOneFails)
{
    defineSafeStoreObjects({ { 1111, "objectId_1", {} },
                             { 2222,
                               "objectId_2",
                               {
                                   .name = "name_2",
                                   .location = "/location_2",
                               } } });
    EXPECT_CALL(*mockSafeStoreWrapper_, getObjectLocation(HasRawPointer(1111))).WillRepeatedly(Return(""));

    EXPECT_CALL(*mockMetadataRescanClientSocket_, rescan)
        .WillOnce(Return(scan_messages::MetadataRescanResponse::undetected));
    EXPECT_CALL(mockSafeStoreResources_, CreateMetadataRescanClientSocket)
        .WillOnce(Return(ByMove(std::move(mockMetadataRescanClientSocket_))));

    MoveFileSystemMocks();
    auto quarantineManager = createQuarantineManager();

    quarantineManager.rescanDatabase();
    EXPECT_TRUE(appenderContains("WARN - Couldn't get path for 'objectId_1': Couldn't get object location"));
    EXPECT_TRUE(appenderContains(
        "INFO - Requesting metadata rescan of quarantined file (original path '/location_2/name_2', object ID 'objectId_2')"));
}
