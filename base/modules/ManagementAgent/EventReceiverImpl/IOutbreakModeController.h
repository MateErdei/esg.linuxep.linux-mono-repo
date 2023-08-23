// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Event.h"

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
        [[nodiscard]] virtual bool processEvent(const Event& event) = 0;

        [[nodiscard]] virtual bool outbreakMode() const = 0;

        virtual void processAction(const std::string& actionXml) = 0;
    };

    using IOutbreakModeControllerPtr = std::shared_ptr<IOutbreakModeController>;
} // namespace ManagementAgent::EventReceiverImpl