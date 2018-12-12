/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/PluginApi/IEventVisitorCallback.h>

namespace Common
{
    namespace PluginApiImpl
    {
    class AbstractEventVisitor : public virtual Common::PluginApi::IEventVisitorCallback
        {
        public:
            virtual ~AbstractEventVisitor() = default;

            void processEvent(Common::EventTypes::CredentialEvent event) override;
            void processEvent(Common::EventTypes::PortScanningEvent event) override;
            void receiveData(const std::string& key, const std::string& data) override;
        };
    }
}