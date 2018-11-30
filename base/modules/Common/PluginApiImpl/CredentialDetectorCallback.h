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
    class CredentialDetectorCallback : public Common::PluginApiImpl::AbstractRawDataCallback, Common::PluginApi::IRawDataCallback
        {
        public:
            Common::EventTypesImpl::CredentialEvent processIncomingCredentialData() override;
            void receiveData(const std::string& key, const std::string& data) override;

        private:
            std::string m_eventId;
            std::string m_data;
        };
    }
}
