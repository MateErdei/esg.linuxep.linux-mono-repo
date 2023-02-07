// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventUtils.h"

void ManagementAgent::EventReceiverImpl::sendEvent(const Event& event)
{
    event.send();
}
