/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "RawDataPublisher.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "PluginResourceManagement.h"

namespace Common
{
    namespace PluginApiImpl
    {

        void RawDataPublisher::sendData(const std::string &rawDataCategory, const std::string &rawData)
        {
            m_socketPublisher->write({rawDataCategory, rawData});
        }

        void sendEvent(const Common::EventTypes::IEventType& event)
        {
            Common::PluginApiImpl::PluginResourceManagement pluginResourceManagement;
            std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
            rawDataPublisher = pluginResourceManagement.createRawDataPublisher();
            rawDataPublisher->sendData(event.getEventTypeId(),event.toString());
        }

        RawDataPublisher::RawDataPublisher(Common::ZeroMQWrapper::ISocketPublisherPtr socketPublisher)
        : m_socketPublisher(std::move(socketPublisher))
        {

        }
    }

}
