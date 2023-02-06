// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>
#include <string>

namespace ManagementAgent::EventReceiverImpl
{
    class IOutbreakModeController
    {
    public:
        virtual ~IOutbreakModeController() = default;

        /**
         * Process an event:
         * * If it's countable count it.
         * * In outbreak mode, if blockable block/drop it
         *
         * @param appId         IN Application ID of event
         * @param eventXml      IN XML contents of event
         * @return True if we should drop the event and not send to Central
         */
        virtual bool processEvent(
            const std::string& appId,
            const std::string& eventXml
            ) = 0;

        [[nodiscard]] virtual bool outbreakMode() const = 0;

    };

    using IOutbreakModeControllerPtr = std::shared_ptr<IOutbreakModeController>;
}