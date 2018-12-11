/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypesImpl/CredentialEvent.h>
#include <Common/EventTypesImpl/PortScanningEvent.h>
#include <Common/PluginApi/IRawDataCallback.h>
#include <Common/PluginApi/IEventVisitorCallback.h>

namespace Common
{
    namespace PluginApiImpl
    {

    class AbstractRawDataCallback : public virtual Common::PluginApi::IEventVisitorCallback
        {
        public:
            virtual ~AbstractRawDataCallback() = default;

            void receiveData(const std::string& key, const std::string& data) override;
            
            virtual void processEvent(Common::EventTypes::ICredentialEventPtr event) override;
            virtual void processEvent(Common::EventTypes::IPortScanningEventPtr event) override;
        };
    }
}