/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/PluginApi/IEventVisitorCallback.h>

namespace Common
{
    namespace PluginApi
    {
    class AbstractEventVisitor : public virtual Common::PluginApi::IEventVisitorCallback
        {
        public:
            virtual ~AbstractEventVisitor() = default;

            void processEvent(const Common::EventTypes::CredentialEvent & event) override;
            void processEvent(const Common::EventTypes::PortScanningEvent & event) override;
            void receiveData(const std::string& key, const std::string& data) override;
        };
    }
}