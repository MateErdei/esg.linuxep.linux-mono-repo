/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IRawDataPublisher.h>
#include <Common/ZeroMQWrapper/ISocketPublisherPtr.h>

namespace Common
{
    namespace PluginApiImpl
    {
        class RawDataPublisher : public virtual Common::PluginApi::IRawDataPublisher
        {
        public:
            RawDataPublisher(Common::ZeroMQWrapper::ISocketPublisherPtr socketPublisher);
            void sendData(const std::string& rawDataCategory, const std::string& rawData) override;

        private:
            Common::ZeroMQWrapper::ISocketPublisherPtr m_socketPublisher;
        };
    }
}



