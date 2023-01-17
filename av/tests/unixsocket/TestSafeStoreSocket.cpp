// Copyright 2022 Sophos Limited. All rights reserved.

#include "SafeStoreSocketMemoryAppenderUsingTests.h"
#include "UnixSocketMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "safestore/MockIQuarantineManager.h"
#include "safestore/SafeStoreServiceCallback.h"
#include "scan_messages/ThreatDetected.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "tests/scan_messages/SampleThreatDetected.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <thirdparty/nlohmann-json/json.hpp>

using namespace testing;
using namespace scan_messages;
namespace fs = sophos_filesystem;
using namespace common::CentralEnums;
using json = nlohmann::json;

namespace
{
    class TestSafeStoreSocket : public SafeStoreSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSafeStoreConfig();
            m_socketPath = Plugin::getSafeStoreSocketPath();
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        static void addCommonSafeStoreTelemetrySetup()
        {
            auto mockFileSystem = new StrictMock<MockFileSystem>();
            std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
            Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
            std::vector<std::string> fileList{ "safestore.db", "safestore.pw" };

            EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillRepeatedly(Return(false));
            EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillRepeatedly(Return(fileList));
            EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(150));
        }

        void sendThreatEventAndVerifyQuarantineTelemetry(
            WaitForEvent& serverWaitGuard,
            safestore::SafeStoreServiceCallback& safeStoreCallback,
            int quarantineSuccessValue,
            int quarantineFailureValue,
            int unlinkFailureValue)
        {
            unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);

            client.sendQuarantineRequest(createThreatDetectedWithRealFd({}));
            client.waitForResponse();
            serverWaitGuard.wait();

            json quarantineTelemetry = json::parse(safeStoreCallback.getTelemetry());
            EXPECT_EQ(quarantineTelemetry["quarantine-successes"], quarantineSuccessValue);
            EXPECT_EQ(quarantineTelemetry["quarantine-failures"], quarantineFailureValue);
            EXPECT_EQ(quarantineTelemetry["unlink-failures"], unlinkFailureValue);
        }

        Common::Threads::NotifyPipe m_notifyPipe{};
        std::string m_socketPath;
    };

    class TestSafeStoreClientSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSafeStoreConfig();
            m_socketPath = Plugin::getSafeStoreSocketPath();
        }

        Common::Threads::NotifyPipe m_notifyPipe{};
        std::string m_socketPath;
    };
} // namespace

TEST_F(TestSafeStoreSocket, TestSendThreatDetected) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    {
        WaitForEvent serverWaitGuard;

        auto quarantineManager = std::make_shared<MockIQuarantineManager>();
        EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                [&serverWaitGuard](
                    const std::string& /*filePath*/,
                    const std::string& /*threatId*/,
                    const std::string& /*threatName*/,
                    const std::string& /*sha256*/,
                    const std::string& /*correlationId*/,
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

        auto threatDetected = createThreatDetectedWithRealFd({});
        client.sendQuarantineRequest(threatDetected);

        serverWaitGuard.wait();

        // destructor will stop the thread
    }

    EXPECT_TRUE(appenderContains("Connected to SafeStore"));
    EXPECT_TRUE(appenderContains("Sending quarantine request to SafeStore for file: /path/to/eicar.txt"));
    EXPECT_TRUE(appenderContains("SafeStore Server thread got connection"));
    EXPECT_TRUE(appenderContains("Read capn of"));
    EXPECT_TRUE(appenderContains("Managed to get file descriptor:"));
    EXPECT_TRUE(appenderContains("Received Threat:"));
    EXPECT_TRUE(appenderContains("File path: /path/to/eicar.txt"));
    EXPECT_TRUE(appenderContains("Threat ID: 01234567-89ab-cdef-0123-456789abcdef"));
    EXPECT_TRUE(appenderContains("Threat name: EICAR-AV-Test"));
    EXPECT_TRUE(appenderContains("SHA256: 131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"));
    EXPECT_TRUE(appenderContains("File descriptor:"));
}

TEST_F(TestSafeStoreSocket, TestSendTwoThreatDetecteds) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;
    WaitForEvent serverWaitGuard2;

    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
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

    client.sendQuarantineRequest(createThreatDetectedWithRealFd({}));
    EXPECT_EQ(client.waitForResponse(), common::CentralEnums::QuarantineResult::NOT_FOUND);
    serverWaitGuard.wait();

    client2.sendQuarantineRequest(createThreatDetectedWithRealFd({}));
    EXPECT_EQ(client2.waitForResponse(), common::CentralEnums::QuarantineResult::NOT_FOUND);
    serverWaitGuard2.wait();

    // destructor will stop the thread
}

TEST_F(TestSafeStoreSocket, SafeStoreTelemetryReturnsExpectedDataAfterSuccessfulQuarantine)
{
    setupFakeSophosThreatDetectorConfig();
    safestore::SafeStoreServiceCallback safeStoreCallback{};
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
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
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
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
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent serverWaitGuard;

    addCommonSafeStoreTelemetrySetup();
    json initialTelemetry = json::parse(safeStoreCallback.getTelemetry());

    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
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

TEST_F(TestSafeStoreSocket, TestSendInvalidData)
{
    // This is a simple test to verify client throws exception on invalid data
    // It is not possible to test the server because BaseServerSocket doesn't expose the socket publicly

    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    unixsocket::SafeStoreServerSocket server(m_socketPath, nullptr);
    server.start();

    unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);

    auto threatDetected = createThreatDetectedWithRealFd({ .threatId = "invalid threat id" });
    EXPECT_ANY_THROW(client.sendQuarantineRequest(threatDetected));
}

TEST_F(TestSafeStoreSocket, TestSendThreatDetectedReceiveResponse)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _, _))
        .WillOnce(Return(common::CentralEnums::QuarantineResult::NOT_FOUND));

    unixsocket::SafeStoreServerSocket server(m_socketPath, quarantineManager);
    server.start();

    unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe);

    auto threatDetected = createThreatDetectedWithRealFd({});
    client.sendQuarantineRequest(threatDetected);

    EXPECT_EQ(client.waitForResponse(), common::CentralEnums::QuarantineResult::NOT_FOUND);

    EXPECT_TRUE(appenderContains("Sending quarantine request to SafeStore for file: /path/to/eicar.txt"));
    EXPECT_TRUE(appenderContains("Sent quarantine request to SafeStore"));
    EXPECT_TRUE(appenderContains("Received quarantine result from SafeStore: failure"));
}

TEST_F(TestSafeStoreClientSocket, testClientSocketTriesToReconnect)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::SafeStoreClient client(m_socketPath, m_notifyPipe, std::chrono::seconds{ 0 });
    EXPECT_FALSE(client.isConnected());

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore - retrying upto 10 times with a sleep of 0s", 1));
    EXPECT_TRUE(appenderContains("Reached the maximum number of attempts connecting to SafeStore"));
}