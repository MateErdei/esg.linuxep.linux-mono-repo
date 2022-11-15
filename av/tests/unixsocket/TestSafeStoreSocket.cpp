// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "safestore/SafeStoreServiceCallback.h"
#include "scan_messages/ThreatDetected.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "common/ApplicationPaths.h"

#include <sys/fcntl.h>
#include <thirdparty/nlohmann-json/json.hpp>

using namespace testing;
using namespace scan_messages;
namespace fs = sophos_filesystem;
using namespace common::CentralEnums;
using json = nlohmann::json;

namespace
{
    class TestSafeStoreSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSafeStoreConfig();
            m_socketPath = Plugin::getSafeStoreSocketPath();
            m_userID = std::getenv("USER");
            m_threatName = "EICAR";
            m_threatPath = "/path/to/unit-test-eicar";
            m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
            m_threatId = "Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350";
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        [[nodiscard]] ThreatDetected createThreatDetected() const
        {
            std::time_t detectionTimeStamp = std::time(nullptr);

            return {
                m_userID,
                detectionTimeStamp,
                ThreatType::virus,
                m_threatName,
                E_SCAN_TYPE_ON_ACCESS_OPEN,
                E_NOTIFICATION_STATUS_CLEANED_UP,
                m_threatPath,
                E_SMT_THREAT_ACTION_SHRED,
                m_sha256,
                m_threatId,
                false,
                ReportSource::ml,
                datatypes::AutoFd(open("/dev/zero", O_RDONLY))};
        }

        static void addCommonSafeStoreTelemetrySetup()
        {
            auto mockFileSystem = new StrictMock<MockFileSystem>();
            std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
            Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
            std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

            EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillRepeatedly(Return(false));
            EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillRepeatedly(Return(fileList));
            EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(150));
        }

        void sendThreatEventAndVerifyQuarantineTelemetry(WaitForEvent& serverWaitGuard,
                                                         safestore::SafeStoreServiceCallback& safeStoreCallback,
                                                         int quarantineSuccessValue,
                                                         int quarantineFailureValue,
                                                         int unlinkFailureValue)
        {
            unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);

            client.sendQuarantineRequest(createThreatDetected());
            client.waitForResponse();
            serverWaitGuard.wait();

            json quarantineTelemetry = json::parse(safeStoreCallback.getTelemetry());
            EXPECT_EQ(quarantineTelemetry["quarantine-successes"], quarantineSuccessValue);
            EXPECT_EQ(quarantineTelemetry["quarantine-failures"], quarantineFailureValue);
            EXPECT_EQ(quarantineTelemetry["unlink-failures"], unlinkFailureValue);
        }

        Common::Threads::NotifyPipe m_notifyPipe {};
        std::string m_socketPath;
        std::string m_threatPath;
        std::string m_threatName;
        std::string m_userID;
        std::string m_sha256;
        std::string m_threatId;
    };

    class MockQuarantineManager : public safestore::QuarantineManager::IQuarantineManager
    {
    public:
        MOCK_METHOD(
            common::CentralEnums::QuarantineResult,
            quarantineFile,
            (const std::string& filePath,
             const std::string& threatId,
             const std::string& threatName,
             const std::string& sha256,
             datatypes::AutoFd autoFd));

        MOCK_METHOD(void, setState,(const safestore::QuarantineManager::QuarantineManagerState& newState));
        MOCK_METHOD(void, initialise,());
        MOCK_METHOD(safestore::QuarantineManager::QuarantineManagerState, getState,());
        MOCK_METHOD(bool, deleteDatabase,());
    };
} // namespace

TEST_F(TestSafeStoreSocket, TestSendThreatDetected) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    {
        WaitForEvent serverWaitGuard;

        auto quarantineManager = std::make_shared<MockQuarantineManager>();
        EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                [&serverWaitGuard](
                    const std::string& /*filePath*/,
                    const std::string& /*threatId*/,
                    const std::string& /*threatName*/,
                    const std::string& /*sha256*/,
                    datatypes::AutoFd autoFd)
                {
                    EXPECT_TRUE(autoFd.valid()); // check we received a valid fd
                    serverWaitGuard.onEventNoArgs();
                    return common::CentralEnums::QuarantineResult::NOT_FOUND;
                }));

        unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);

        server.start();

        // connect after we start
        unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);

        auto threatDetected = createThreatDetected();
        client.sendQuarantineRequest(threatDetected);

        serverWaitGuard.wait();

        // destructor will stop the thread
    }

    EXPECT_TRUE(appenderContains("SafeStore Server starting listening on socket:"));
    EXPECT_TRUE(appenderContains("SafeStore Server accepting connection:"));
    EXPECT_TRUE(appenderContains("SafeStore Server thread got connection"));
    EXPECT_TRUE(appenderContains("Read capn of"));
    EXPECT_TRUE(appenderContains("Managed to get file descriptor:"));
    EXPECT_TRUE(appenderContains("Received Threat:"));
    EXPECT_TRUE(appenderContains("File path: " + m_threatPath));
    EXPECT_TRUE(appenderContains("Threat ID: " + m_threatId));
    EXPECT_TRUE(appenderContains("Threat name: " + m_threatName));
    EXPECT_TRUE(appenderContains("SHA256: " + m_sha256));
    EXPECT_TRUE(appenderContains("File descriptor:"));
    EXPECT_TRUE(appenderContains("Closing SafeStore Server socket"));
}

TEST_F(TestSafeStoreSocket, testClientSocketTriesToReconnect) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe, std::chrono::seconds{0});

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}

TEST_F(TestSafeStoreSocket, TestSendTwoThreatDetecteds) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;
    WaitForEvent serverWaitGuard2;

    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
        .Times(2)
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }))
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard2]()
            {
                serverWaitGuard2.onEventNoArgs();
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }));

    unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);

    server.start();

    // connect after we start
    unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);
    unixsocket::SafeStoreClient client2(m_socketPath, m_notifyPipe);

    client.sendQuarantineRequest(createThreatDetected());
    client.waitForResponse();
    serverWaitGuard.wait();

    client2.sendQuarantineRequest(createThreatDetected());
    client2.waitForResponse();
    serverWaitGuard2.wait();

    // destructor will stop the thread
}

TEST_F(TestSafeStoreSocket, SafeStoreTelemetryReturnsExpectedDataAfterSuccessfulQuarantine)
{
    setupFakeSophosThreatDetectorConfig();
    safestore::SafeStoreServiceCallback safeStoreCallback{};
    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::SUCCESS;
            }));
    unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);
    server.start();

    EXPECT_EQ(initialTelemetry["quarantine-successes"], 0);
    sendThreatEventAndVerifyQuarantineTelemetry(serverWaitGuard, safeStoreCallback, 1, 0, 0);
}

TEST_F(TestSafeStoreSocket, SafeStoreTelemetryReturnsExpectedDataAfterFailedQuarantine)
{
    setupFakeSophosThreatDetectorConfig();
    safestore::SafeStoreServiceCallback safeStoreCallback{};
    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::NOT_FOUND;
            }));
    unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);
    server.start();

    EXPECT_EQ(initialTelemetry["quarantine-failures"], 0);
    sendThreatEventAndVerifyQuarantineTelemetry(serverWaitGuard, safeStoreCallback, 0, 1, 0);
}

TEST_F(TestSafeStoreSocket, SafeStoreTelemetryReturnsExpectedDataAfterUnlinkFailure)
{
    setupFakeSophosThreatDetectorConfig();
    safestore::SafeStoreServiceCallback safeStoreCallback{};
    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::FAILED_TO_DELETE_FILE;
            }));
    unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);
    server.start();

    EXPECT_EQ(initialTelemetry["unlink-failures"], 0);
    sendThreatEventAndVerifyQuarantineTelemetry(serverWaitGuard, safeStoreCallback, 0, 0, 1);
}
