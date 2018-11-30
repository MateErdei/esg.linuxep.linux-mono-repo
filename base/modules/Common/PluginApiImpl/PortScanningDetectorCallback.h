/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IRawDataCallback.h>
#include <Common/PluginApiImpl/AbstractRawDataCallback.h>

namespace Common
{

    namespace PluginApiImpl
    {
        class PortScanningDetectorCallback : public Common::PluginApiImpl::AbstractRawDataCallback, Common::PluginApi::IRawDataCallback
        {
        public:
            Common::EventTypesImpl::PortScanningEvent processIncomingPortScanningData() override;
            void receiveData(const std::string& key, const std::string& data) override;
        private:
            std::string m_data;
            std::string m_eventTypeId;
        };
    }
}
