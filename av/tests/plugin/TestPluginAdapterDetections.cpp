// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"

#include "datatypes/UuidGeneratorImpl.h"
#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/StringUtils.h"
#include "scan_messages/SampleThreatDetected.h"

// Test code
#include "tests/common/SetupFakePluginDir.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "tests/datatypes/MockUuidGenerator.h"
#include "Common/Helpers/MockApiBaseServices.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <utility>

using namespace ::testing;
using namespace Plugin;
using namespace common::CentralEnums;

namespace fs = sophos_filesystem;

namespace
{
    class TestPluginAdapterDetections : public PluginMemoryAppenderUsingTests
    {
    protected:
        TestPluginAdapterDetections() :
            m_tempDir{ setupFakePluginDir() },
            m_taskQueue{ std::make_shared<TaskQueue>() },
            m_callback{ std::make_shared<PluginCallback>(m_taskQueue) },
            m_threatEventPublisherSocketPath{ pluginInstall() / "threatEventPublisherSocket" },
            m_mockUuidGenerator{ std::make_unique<MockUuidGenerator>() },
            m_mockApiBaseServices{ std::make_unique<MockApiBaseServices>() },
            m_memoryAppenderHolder{ *this }
        {
            Common::Telemetry::TelemetryHelper::getInstance().reset();
        }

        std::unique_ptr<Tests::TempDir> m_tempDir;
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::shared_ptr<PluginCallback> m_callback;
        std::string m_threatEventPublisherSocketPath;
        std::unique_ptr<MockUuidGenerator> m_mockUuidGenerator;
        std::unique_ptr<MockApiBaseServices> m_mockApiBaseServices;
        UsingMemoryAppender m_memoryAppenderHolder;

        std::string m_threatId1 = "00000000-0000-0000-0000-000000000000";
        std::string m_threatId2 = "11111111-1111-1111-1111-111111111111";
    };
} // namespace

TEST_F(TestPluginAdapterDetections, UnquarantinedDetectionSendsEventAndBadHealth)
{
    auto detection =
        createThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "" });

    // We expect to generate a new correlation ID as this detection wasn't seen before
    EXPECT_CALL(*m_mockUuidGenerator, generate()).Times(1).WillOnce(Return("correlationId"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially, then to bad
    {
        InSequence seq;
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":2}")).Times(1);
    }

    // After receiving the detection, plugin should send events
    auto detectionCentral = createThreatDetected(
        { .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "correlationId" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral)))
        .Times(1);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral)))
        .Times(1);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.finaliseDetection(detection);
    EXPECT_EQ(detection.correlationId, "correlationId");

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_SUSPICIOUS);
    EXPECT_TRUE(appenderContains("Threat database is not empty, sending suspicious health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 1));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 1));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 1));
}

TEST_F(TestPluginAdapterDetections, QuarantinedDetectionSendsEventsButNoNewHealth)
{
    auto detection = createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "" });

    // We expect to generate a new correlation ID as this detection wasn't seen before
    EXPECT_CALL(*m_mockUuidGenerator, generate()).Times(1).WillOnce(Return("correlationId"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially, but only once on startup
    EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    // After receiving the detection, plugin should send events
    const auto detectionCentral =
        createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "correlationId" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral)))
        .Times(1);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral)))
        .Times(1);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.finaliseDetection(detection);
    EXPECT_EQ(detection.correlationId, "correlationId");

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 1));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 1));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 1));
}

namespace
{
    class PluginAdapterWithCatch : public PluginAdapter
    {
    public:
        using PluginAdapter::PluginAdapter;
        std::exception_ptr thrownException_;


        void mainLoopWithCatch()
        {
            try
            {
                mainLoop();
            }
            catch (const Common::Exceptions::IException& ex)
            {
                PRINT("Main Loop threw exception!: " << ex.what_with_location());
                thrownException_ = std::current_exception();
                throw;
            }
            catch (const std::exception& ex)
            {
                PRINT("Main Loop threw std::exception!: " << ex.what());
                thrownException_ = std::current_exception();
                throw;
            }
        }
    };
}

TEST_F(
    TestPluginAdapterDetections,
    DetectionThatSucceedsToQuarantineAfterFailingResolvesBadHealthAndSameCorrelationIdIsUsed)
{
    auto detection1 =
        createThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "" });

    auto detection2 = createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "" });

    // We expect to generate a new correlation ID once only
    EXPECT_CALL(*m_mockUuidGenerator, generate()).Times(1).WillOnce(Return("correlationId"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially, then bad on failure, then good again on success
    {
        InSequence seq;
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":2}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    }

    // The events for failure should be sent
    ExpectationSet failureSet;
    const auto detectionCentral1 = createThreatDetected(
        { .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "correlationId" });
    failureSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral1)))
            .Times(1);
    failureSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral1)))
            .Times(1);

    // After which the events for success will be sent
    const auto detectionCentral2 =
        createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "correlationId" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral2)))
        .Times(1)
        .After(failureSet);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral2)))
        .Times(1)
        .After(failureSet);

    // Ignore requests for policies
    EXPECT_CALL(*m_mockApiBaseServices, requestPolicies(_)).WillRepeatedly(Return());

    PluginAdapterWithCatch pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapterWithCatch::mainLoopWithCatch, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.finaliseDetection(detection1);
    EXPECT_EQ(detection1.correlationId, "correlationId");

    pluginAdapter.finaliseDetection(detection2);
    EXPECT_EQ(detection2.correlationId, "correlationId");

    m_taskQueue->pushStop();
    ASSERT_TRUE(pluginThread.joinable());
    pluginThread.join();
    EXPECT_EQ(pluginAdapter.thrownException_, nullptr);

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_TRUE(appenderContains("Threat database is not empty, sending suspicious health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Threat database is now empty, sending good health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 2));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 2));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 2));
}

TEST_F(TestPluginAdapterDetections, RedetectionAfterSuccessfulQuarantineGeneratesNewCorrelationId)
{
    auto detection1 = createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "" });
    auto detection2 = createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "" });

    // We expect separate correlation IDs for these detections
    EXPECT_CALL(*m_mockUuidGenerator, generate())
        .Times(2)
        .WillOnce(Return("correlationId1"))
        .WillOnce(Return("correlationId2"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially
    EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    // First we get events for the first detection with the first correlation id
    const auto detectionCentral1 =
        createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "correlationId1" });
    ExpectationSet firstDetectionSet;
    firstDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral1)))
            .Times(1);
    firstDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral1)))
            .Times(1);

    // Then we get events for the second detection with the second correlation id
    const auto detectionCentral2 =
        createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "correlationId2" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral2)))
        .Times(1)
        .After(firstDetectionSet);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral2)))
        .Times(1)
        .After(firstDetectionSet);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.finaliseDetection(detection1);
    EXPECT_EQ(detection1.correlationId, "correlationId1");

    pluginAdapter.finaliseDetection(detection2);
    EXPECT_EQ(detection2.correlationId, "correlationId2");

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 2));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 2));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 2));
}

TEST_F(
    TestPluginAdapterDetections,
    QuarantinedDetectionAfterUnquarantinedDetectionWithDifferentThreatIdDoesntResolveBadHealthAndUsesDifferentCorrelationId)
{
    auto detection1 = createThreatDetected(
        { .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .threatId = m_threatId1, .correlationId = "" });
    auto detection2 = createThreatDetected(
        { .quarantineResult = QuarantineResult::SUCCESS, .threatId = m_threatId2, .correlationId = "" });

    // We expect separate correlation IDs for these detections
    EXPECT_CALL(*m_mockUuidGenerator, generate())
        .Times(2)
        .WillOnce(Return("correlationId1"))
        .WillOnce(Return("correlationId2"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially, then bad health on first detection, but the success on second doesn't
    // reset it back
    {
        InSequence seq;
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":2}")).Times(1);
    }

    // First detection
    const auto detectionCentral1 = createThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE,
                                                          .threatId = m_threatId1,
                                                          .correlationId = "correlationId1" });
    ExpectationSet firstDetectionSet;
    firstDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral1)))
            .Times(1);
    firstDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral1)))
            .Times(1);

    // The success will happen after the failure
    const auto detectionCentral2 = createThreatDetected(
        { .quarantineResult = QuarantineResult::SUCCESS, .threatId = m_threatId2, .correlationId = "correlationId2" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral2)))
        .Times(1)
        .After(firstDetectionSet);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral2)))
        .Times(1)
        .After(firstDetectionSet);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup

    pluginAdapter.finaliseDetection(detection1);
    EXPECT_EQ(detection1.correlationId, "correlationId1");

    pluginAdapter.finaliseDetection(detection2);
    EXPECT_EQ(detection2.correlationId, "correlationId2");

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_SUSPICIOUS);
    EXPECT_TRUE(appenderContains("Threat database is not empty, sending suspicious health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 2));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 2));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 2));
}

TEST_F(
    TestPluginAdapterDetections,
    MarkingDetectionAsBeingQuarantinedCreatesACorrelationIdWhichIsUsedByIntermediateDetection)
{
    auto detection1 = createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "" });
    auto detection2 =
        createThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "" });

    // We expect these to use the same correlation ID
    EXPECT_CALL(*m_mockUuidGenerator, generate()).Times(1).WillOnce(Return("correlationId"));
    datatypes::UuidGeneratorReplacer uuidGeneratorReplacer{ std::move(m_mockUuidGenerator) };

    // Plugin should send good health initially, then bad, then good once quarantine finishes
    {
        InSequence seq;
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":2}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    }

    // First we get events for the second detection and bad health, as the first hasn't finished quarantining
    const auto detectionCentral2 = createThreatDetected(
        { .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE, .correlationId = "correlationId" });
    ExpectationSet secondDetectionSet;
    secondDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral2)))
            .Times(1);
    secondDetectionSet +=
        EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral2)))
            .Times(1);

    // Then we get events for the first detection, and good health again
    const auto detectionCentral1 =
        createThreatDetected({ .quarantineResult = QuarantineResult::SUCCESS, .correlationId = "correlationId" });
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateThreatDetectedXml(detectionCentral1)))
        .Times(1)
        .After(secondDetectionSet);
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", pluginimpl::generateCoreCleanEventXml(detectionCentral1)))
        .Times(1)
        .After(secondDetectionSet);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.markAsQuarantining(detection1);
    EXPECT_EQ(detection1.correlationId, "correlationId");

    pluginAdapter.finaliseDetection(detection2);
    EXPECT_EQ(detection2.correlationId, "correlationId");

    pluginAdapter.finaliseDetection(detection1);
    EXPECT_EQ(detection1.correlationId, "correlationId");

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_TRUE(appenderContains("Threat database is not empty, sending suspicious health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Threat database is now empty, sending good health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 2));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 2));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 2));
}

TEST_F(TestPluginAdapterDetections, SecondUnquarantinedDetectionIsNotReportedToCentralAndDoesntChangeHealth)
{
    auto detection = createThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE });

    {
        InSequence seq;
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
        EXPECT_CALL(*m_mockApiBaseServices, sendThreatHealth("{\"ThreatHealth\":2}")).Times(1);
    }

    // We expect 2 events (detection+clean) rather than 4
    EXPECT_CALL(*m_mockApiBaseServices, sendEvent("CORE", _)).Times(2);

    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    std::thread pluginThread{ &PluginAdapter::mainLoop, &pluginAdapter };
    ASSERT_TRUE(waitForLog("Publishing threat health: good")); // Make sure we are past the startup
    clearMemoryAppender();

    pluginAdapter.finaliseDetection(detection);
    pluginAdapter.finaliseDetection(detection);

    m_taskQueue->pushStop();
    pluginThread.join();

    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_SUSPICIOUS);
    EXPECT_TRUE(appenderContains("Threat database is not empty, sending suspicious health to Management agent", 1));
    EXPECT_TRUE(appenderContains("Sending threat detection notification to central: ", 1));
    EXPECT_TRUE(appenderContains("Publishing threat detection event: ", 1));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a new detection", 1));
    EXPECT_TRUE(appenderContains("Found 'EICAR-AV-Test' in '/path/to/eicar.txt' which is a duplicate detection", 1));
}

TEST_F(TestPluginAdapterDetections, telemetryIsIncrementedForNewDetection)
{
    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    auto detection = createOnAccessThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE });
    pluginAdapter.finaliseDetection(detection);

    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-access-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapterDetections, telemetryIsIncrementedForDuplicateDetection)
{
    PluginAdapter pluginAdapter{
        m_taskQueue, std::move(m_mockApiBaseServices), m_callback, m_threatEventPublisherSocketPath
    };

    auto detection = createOnAccessThreatDetected({ .quarantineResult = QuarantineResult::FAILED_TO_DELETE_FILE });
    pluginAdapter.finaliseDetection(detection);
    pluginAdapter.finaliseDetection(detection);

    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-access-threat-eicar-count"], 2);
}