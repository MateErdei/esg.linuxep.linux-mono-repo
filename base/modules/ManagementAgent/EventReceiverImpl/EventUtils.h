// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace ManagementAgent::EventReceiverImpl
{
    /**
     * Send event to mcsrouter via filesystem
     * @param appId
     * @param eventXml
     */
    void sendEvent(
        const std::string& appId,
        const std::string& eventXml
        );
}