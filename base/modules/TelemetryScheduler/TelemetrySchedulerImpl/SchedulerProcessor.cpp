/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerTask.h"

namespace TelemetrySchedulerImpl
{
    SchedulerProcessor::SchedulerProcessor(std::shared_ptr<TaskQueue> taskQueue) :
        m_taskQueue(std::move(taskQueue))
    {
    }

    void SchedulerProcessor::run()
    {
        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task)
            {
                case Task::WaitToRunTelemetry:
                    break; // TODO: LINUXEP-6639 handle scheduling of next run of telelmetry executable

                case Task::RunTelemetry:
                    break; // TODO: LINUXEP-7984 run telelmetry executable

                case Task::Shutdown:
                    return;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }
} // namespace TelemetrySchedulerImpl
