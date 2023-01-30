// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>

namespace ManagementAgent::EventReceiverImpl
{
    class IOutbreakModeController
    {
    public:
        virtual ~IOutbreakModeController() = default;

        /**
         *
         * @param appId
         * @param eventXml
         * @return True if we should drop the event and not send to Central
         */
        virtual bool recordEventAndDetermineIfItShouldBeDropped(
            const std::string& appId,
            const std::string& eventXml
            ) = 0;

    };

    using IOutbreakModeControllerPtr = std::shared_ptr<IOutbreakModeController>;
}