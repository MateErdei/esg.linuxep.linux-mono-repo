// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventTask.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

ManagementAgent::EventReceiverImpl::EventTask::EventTask(
    ManagementAgent::EventReceiverImpl::Event event) :
    event_(std::move(event))
{
}

void ManagementAgent::EventReceiverImpl::EventTask::run()
{
    event_.send();
}