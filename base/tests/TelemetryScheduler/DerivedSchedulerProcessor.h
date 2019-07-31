/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <gtest/gtest.h>
#include <modules/Common/TelemetryExeConfigImpl/Config.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>

using namespace std::chrono;
using namespace std::chrono_literals;

class DerivedSchedulerProcessor : public TelemetrySchedulerImpl::SchedulerProcessor
{
public:
    DerivedSchedulerProcessor(
        std::shared_ptr<TelemetrySchedulerImpl::TaskQueue> taskQueue,
        const Common::ApplicationConfiguration::IApplicationPathManager& pathManager,
        seconds configurationCheckDelay = 3600s,
        seconds telemetryExeCheckDelay = 60s) :
        SchedulerProcessor(std::move(taskQueue), pathManager, configurationCheckDelay, telemetryExeCheckDelay)
    {
    }

    void waitToRunTelemetry(bool runPastScheduledRunNow) override
    {
        SchedulerProcessor::waitToRunTelemetry(runPastScheduledRunNow);
    }

    void runTelemetry() override { SchedulerProcessor::runTelemetry(); }
    void checkExecutableFinished() override { SchedulerProcessor::checkExecutableFinished(); }

    bool delayingTelemetryRun() override { return SchedulerProcessor::delayingTelemetryRun(); }
    bool delayingConfigurationCheck() override { return SchedulerProcessor::delayingConfigurationCheck(); }
};
