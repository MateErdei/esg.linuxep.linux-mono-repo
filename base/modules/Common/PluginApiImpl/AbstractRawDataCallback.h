/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypesImpl/CredentialEvent.h>
#include <Common/EventTypesImpl/PortScanningEvent.h>
#include <Common/PluginApi/IRawDataCallback.h>

namespace Common
{
    namespace PluginApiImpl
    {

    class AbstractRawDataCallback : public Common::PluginApi::IRawDataCallback
        {
        public:
            virtual ~AbstractRawDataCallback() = default;

            void receiveData(const std::string& key, const std::string& data) override;
            
            virtual void processEvent(Common::EventTypesImpl::CredentialEvent& event);
            virtual void processEvent(Common::EventTypesImpl::PortScanningEvent& event);
        };
    }
}