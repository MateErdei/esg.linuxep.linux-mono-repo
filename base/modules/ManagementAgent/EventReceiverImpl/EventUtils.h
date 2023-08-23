// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Event.h"

namespace ManagementAgent::EventReceiverImpl
{
    /**
     * Send event to mcsrouter via filesystem
     * @param event
     */
    void sendEvent(const Event& event);
} // namespace ManagementAgent::EventReceiverImpl