// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOutbreakModeController.h"

namespace ManagementAgent::EventReceiverImpl
{
    class OutbreakModeController : public IOutbreakModeController
    {
    public:
        bool recordEventAndDetermineIfItShouldBeDropped(const std::string& appId, const std::string& eventXml) override;
    };

    using OutbreakModeControllerPtr = std::shared_ptr<OutbreakModeController>;
}
