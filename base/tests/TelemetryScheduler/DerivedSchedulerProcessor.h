/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <gtest/gtest.h>
#include <modules/Common/TelemetryExeConfigImpl/Config.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

class DerivedSchedulerProcessor : public TelemetrySchedulerImpl::SchedulerProcessor
{
public:
    DerivedSchedulerProcessor(
        std::shared_ptr<TelemetrySchedulerImpl::TaskQueue> taskQueue,
        const Common::ApplicationConfiguration::IApplicationPathManager& pathManager) :
        SchedulerProcessor(std::move(taskQueue), pathManager)
    {
    }

    void waitToRunTelemetry() override { SchedulerProcessor::waitToRunTelemetry(); }
    void runTelemetry() override { SchedulerProcessor::runTelemetry(); }
    void checkExecutableFinished() override { SchedulerProcessor::checkExecutableFinished(); }

    bool delayingTelemetryRun() override { return SchedulerProcessor::delayingTelemetryRun(); }
    bool delayingConfigurationCheck() override { return SchedulerProcessor::delayingConfigurationCheck(); }
};
