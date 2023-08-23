// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Event.h"
#include "IOutbreakModeController.h"

#include "Common/TaskQueue/ITask.h"

#include <string>

namespace ManagementAgent::EventReceiverImpl
{
    class EventTask : public virtual Common::TaskQueue::ITask
    {
    public:
        EventTask(Event event, IOutbreakModeControllerPtr outbreakModeController);
        void run() override;

    private:
        Event event_;
        IOutbreakModeControllerPtr outbreakModeController_;
    };
} // namespace ManagementAgent::EventReceiverImpl
