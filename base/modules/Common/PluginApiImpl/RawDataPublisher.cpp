/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "RawDataPublisher.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"

namespace Common
{
    namespace PluginApiImpl
    {

        void RawDataPublisher::sendData(const std::string &rawDataCategory, const std::string &rawData)
        {
            m_socketPublisher->write({rawDataCategory, rawData});
        }
        RawDataPublisher::RawDataPublisher(Common::ZeroMQWrapper::ISocketPublisherPtr socketPublisher)
        : m_socketPublisher(std::move(socketPublisher))
        {

        }
    }

}
