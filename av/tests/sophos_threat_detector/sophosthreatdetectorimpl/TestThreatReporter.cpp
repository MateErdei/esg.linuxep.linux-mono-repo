// Copyright 2021-2022 Sophos Limited. All rights reserved.

#include "scan_messages/SampleThreatDetected.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include "tests/common/Common.h"
#include "tests/common/LogInitializedTests.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/IMessageCallback.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace common::CentralEnums;
namespace fs = sophos_filesystem;

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
} // namespace

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
    unixsocket::ThreatReporterServerSocket threatReporterServer(socket_path, 0600, mockThreatReportCallback);

    threatReporterServer.start();

    sspl::sophosthreatdetectorimpl::ThreatReporter reporterClient(socket_path);

    reporterClient.sendThreatReport(createThreatDetectedWithRealFd({}));

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();
}
