/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <gtest/gtest.h>
#include <modules/Common/TelemetryExeConfigImpl/Config.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <modules/TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>

class DerivedSchedulerProcessor : public TelemetrySchedulerImpl::SchedulerProcessor
{
public:
    DerivedSchedulerProcessor(
        std::shared_ptr<TelemetrySchedulerImpl::TaskQueue> taskQueue,
        const std::string& supplementaryConfigFilePath,
        const std::string& telemetryExeConfigFilePath,
        const std::string& telemetryStatusFilePath) :
        SchedulerProcessor(std::move(taskQueue), supplementaryConfigFilePath, telemetryExeConfigFilePath, telemetryStatusFilePath)
    {
    }

    void waitToRunTelemetry() override { SchedulerProcessor::waitToRunTelemetry(); }
    void runTelemetry() override { SchedulerProcessor::runTelemetry(); }
    size_t getIntervalFromSupplementaryFile() override { return SchedulerProcessor::getIntervalFromSupplementaryFile(); }
    size_t getScheduledTimeUsingSupplementaryFile() override { return SchedulerProcessor::getScheduledTimeUsingSupplementaryFile(); }
};
