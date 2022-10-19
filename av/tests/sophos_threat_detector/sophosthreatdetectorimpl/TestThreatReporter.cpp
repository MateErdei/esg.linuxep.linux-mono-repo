// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "../../common/WaitForEvent.h"
#include "../../common/Common.h"
#include "../../common/LogInitializedTests.h"


#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include <unixsocket/IMessageCallback.h>
#include <unixsocket/threatReporterSocket/ThreatReporterServerSocket.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fcntl.h>

using namespace ::testing;
using namespace common::CentralEnums;

namespace
{
    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(scan_messages::ThreatDetected detection));
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

    EXPECT_CALL(*mockThreatReportCallback, processMessage(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

    fs::path socket_path = pluginInstall() / "chroot/var/threat_report_socket";
    unixsocket::ThreatReporterServerSocket threatReporterServer(
        socket_path, 0600, mockThreatReportCallback
    );

    threatReporterServer.start();

    sspl::sophosthreatdetectorimpl::ThreatReporter reporterClient(socket_path);

    scan_messages::ThreatDetected threatDetected(
        "root",
        1,
        ThreatType::virus,
        "threatName",
        scan_messages::E_SCAN_TYPE_UNKNOWN,
        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
        "/path",
        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
        "sha256",
        "threatId",
        false,
        ReportSource::ml,
        datatypes::AutoFd(open("/dev/zero", O_RDONLY)));

    reporterClient.sendThreatReport(threatDetected);

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();
}
