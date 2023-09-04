// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include <memory>
#include <string>

namespace ManagementAgent::PluginCommunication
{
    class IEventReceiver
    {
    public:
        virtual ~IEventReceiver() = default;

        /**
         * Handle the situation where a plugin is sending an event to the management agent
         *
         * @param appId
         * @param eventXml
         */
        virtual void receivedSendEvent(const std::string& appId, const std::string& eventXml) = 0;

        /**
         * Handle any actions we need to take for recovering from Outbreak mode
         * @param actionXml
         */
        virtual void handleAction(const std::string& actionXml) = 0;

        /**
         * Are we in outbreak mode?
         * @return
         */
        [[nodiscard]] virtual bool outbreakMode() const = 0;
    };

    using IEventReceiverPtr = std::shared_ptr<IEventReceiver>;
} // namespace ManagementAgent::PluginCommunication
