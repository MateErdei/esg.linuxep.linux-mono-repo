// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include "unixsocket/IMessageCallback.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

// test includes
#include "tests/scan_messages/SampleThreatDetected.h"
#include "tests/common/Common.h"
#include "tests/common/LogInitializedTests.h"
#include "tests/common/WaitForEvent.h"

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

TEST_F(TestThreatReporter, testConstruction)
{
    sspl::sophosthreatdetectorimpl::ThreatReporter foo("/bar");
}

TEST_F(TestThreatReporter, testReport)
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
