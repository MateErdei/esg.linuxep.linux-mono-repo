/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../../common/WaitForEvent.h"
#include "../../common/Common.h"
#include "../../common/LogInitializedTests.h"


#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include <unixsocket/IMessageCallback.h>
#include <unixsocket/threatReporterSocket/ThreatReporterServerSocket.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const scan_messages::ServerThreatDetected& detection));
    };

    class TestThreatReporter : public LogInitializedTests
    {
    public:
    };
}


TEST_F(TestThreatReporter, testConstruction) // NOLINT
{
    sspl::sophosthreatdetectorimpl::ThreatReporter foo("/bar");
}

TEST_F(TestThreatReporter, testReport) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;

    auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mockThreatReportCallback, processMessage(_)).Times(1).WillOnce(
        InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

    fs::path socket_path = pluginInstall() / "chroot/var/threat_report_socket";
    unixsocket::ThreatReporterServerSocket threatReporterServer(
        socket_path, 0600, mockThreatReportCallback
    );

    threatReporterServer.start();

    sspl::sophosthreatdetectorimpl::ThreatReporter reporterClient(socket_path);

    reporterClient.sendThreatReport("/path", "threatName", "sha256", 1, "root", 1);

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();
}
