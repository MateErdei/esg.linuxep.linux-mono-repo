// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "common/ApplicationPaths.h"
#include "common/NotifyPipeSleeper.h"
#include "safestore/SafeStoreServiceCallback.h"
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "Common/FileSystem/IFilePermissions.h"

#include "SafeStoreSocketMemoryAppenderUsingTests.h"
#include "UnixSocketMemoryAppenderUsingTests.h"

#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "tests/safestore/MockIQuarantineManager.h"
#include "tests/scan_messages/SampleThreatDetected.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <nlohmann/json.hpp>

using namespace testing;
using namespace scan_messages;
using namespace unixsocket;
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
            SafeStoreClient client(m_socketPath, m_notifyPipe);

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

TEST_F(TestSafeStoreSocket, TestSendThreatDetected)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger baseClientLogger = Common::Logging::getInstance("UnixSocket");
    baseClientLogger.addAppender(m_sharedAppender);
    setupFakeSophosThreatDetectorConfig();

    {
        WaitForEvent serverWaitGuard;

        auto quarantineManager = std::make_shared<MockIQuarantineManager>();
        EXPECT_CALL(*quarantineManager, quarantineFile)
            .Times(1)
            .WillOnce(Invoke(
                [&serverWaitGuard](
                    const std::string& /*filePath*/,
                    const std::string& /*threatId*/,
                    const std::string& /*threatType*/,
                    const std::string& /*threatName*/,
                    const std::string& /*threatSha256*/,
                    const std::string& /*sha256*/,
                    const std::string& /*correlationId*/,
                    datatypes::AutoFd autoFd)
                {
                    EXPECT_TRUE(autoFd.valid()); // check we received a valid fd
                    serverWaitGuard.onEventNoArgs();
                    return common::CentralEnums::QuarantineResult::NOT_FOUND;
                }));

        SafeStoreServerSocket server(m_socketPath, quarantineManager);

        server.start();

        // connect after we start
        SafeStoreClient client(m_socketPath, m_notifyPipe);

        auto threatDetected = createThreatDetectedWithRealFd({});
        client.sendQuarantineRequest(threatDetected);

        serverWaitGuard.wait();

        // destructor will stop the thread
    }

    EXPECT_TRUE(appenderContains("SafeStoreClient connected"));
    EXPECT_TRUE(
        appenderContains("SafeStoreClient sending quarantine request to SafeStore for file: '/path/to/eicar.txt'"));
    EXPECT_TRUE(appenderContains("SafeStoreServerConnectionThread got connection"));
    EXPECT_TRUE(appenderContains("SafeStoreServerConnectionThread read capn of"));
    EXPECT_TRUE(appenderContains("SafeStoreServerConnectionThread managed to get file descriptor:"));
    EXPECT_TRUE(appenderContains("Received Threat:"));
    EXPECT_TRUE(appenderContains("File path: '/path/to/eicar.txt'"));
    EXPECT_TRUE(appenderContains("Threat ID: 01234567-89ab-cdef-0123-456789abcdef"));
    EXPECT_TRUE(appenderContains("Threat type: virus"));
    EXPECT_TRUE(appenderContains("Threat name: EICAR-AV-Test"));
    EXPECT_TRUE(appenderContains("Threat SHA256: 494aaaff46fa3eac73ae63ffbdfd8267131f95c51cc819465fa1797f6ccacf9d"));
    EXPECT_TRUE(appenderContains("SHA256: 131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"));
    EXPECT_TRUE(appenderContains("File descriptor:"));
}

TEST_F(TestSafeStoreSocket, TestSendTwoThreatDetecteds)
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;
    WaitForEvent serverWaitGuard2;

    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, quarantineFile)
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

    SafeStoreServerSocket server(m_socketPath, quarantineManager);

    server.start();

    // connect after we start
    SafeStoreClient client(m_socketPath, m_notifyPipe);
    SafeStoreClient client2(m_socketPath, m_notifyPipe);

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

    EXPECT_CALL(*quarantineManager, quarantineFile)
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::SUCCESS;
            }));
    SafeStoreServerSocket server(m_socketPath, quarantineManager);
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

    EXPECT_CALL(*quarantineManager, quarantineFile)
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::NOT_FOUND;
            }));
    SafeStoreServerSocket server(m_socketPath, quarantineManager);
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

    EXPECT_CALL(*quarantineManager, quarantineFile)
        .WillOnce(InvokeWithoutArgs(
            [&serverWaitGuard]()
            {
                serverWaitGuard.onEventNoArgs();
                return QuarantineResult::FAILED_TO_DELETE_FILE;
            }));
    SafeStoreServerSocket server(m_socketPath, quarantineManager);
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

    SafeStoreServerSocket server(m_socketPath, nullptr);
    server.start();

    SafeStoreClient client(m_socketPath, m_notifyPipe);

    auto threatDetected = createThreatDetectedWithRealFd({ .threatId = "invalid threat id" });
    EXPECT_ANY_THROW(client.sendQuarantineRequest(threatDetected));
}

TEST_F(TestSafeStoreSocket, TestSendThreatDetectedReceiveResponse)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, quarantineFile).WillOnce(Return(common::CentralEnums::QuarantineResult::NOT_FOUND));

    SafeStoreServerSocket server(m_socketPath, quarantineManager);
    server.start();

    SafeStoreClient client(m_socketPath, m_notifyPipe);

    auto threatDetected = createThreatDetectedWithRealFd({});
    client.sendQuarantineRequest(threatDetected);

    EXPECT_EQ(client.waitForResponse(), common::CentralEnums::QuarantineResult::NOT_FOUND);

    EXPECT_TRUE(
        appenderContains("SafeStoreClient sending quarantine request to SafeStore for file: '/path/to/eicar.txt'"));
    EXPECT_TRUE(appenderContains("SafeStoreClient sent quarantine request to SafeStore"));
    EXPECT_TRUE(appenderContains("SafeStoreClient received quarantine result from SafeStore: failure"));
}

TEST_F(TestSafeStoreClientSocket, testClientSocketTriesToReconnect)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    SafeStoreClient client(m_socketPath, m_notifyPipe, std::chrono::seconds{ 0 });
    EXPECT_FALSE(client.isConnected());

    EXPECT_TRUE(appenderContains(
        "SafeStoreClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 0s", 1));
    EXPECT_TRUE(appenderContains("SafeStoreClient reached the maximum number of connection attempts"));
}

TEST_F(TestSafeStoreClientSocket, TestClientSocketTimeOutInterrupted)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe notifyPipe;
    auto notifyPipeSleeper = std::make_unique<common::NotifyPipeSleeper>(notifyPipe);

    std::thread t1(
        [&]() {
            SafeStoreClient client{ m_socketPath, notifyPipe, std::chrono::seconds{ 1 }, std::move(notifyPipeSleeper) };
        });

    EXPECT_TRUE(waitForLog(
        "SafeStoreClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 1s", 2s));
    notifyPipe.notify();

    t1.join();

    EXPECT_TRUE(appenderContains("SafeStoreClient received stop request while connecting"));
    EXPECT_FALSE(appenderContains("SafeStoreClient reached the maximum number of connection attempts"));
}