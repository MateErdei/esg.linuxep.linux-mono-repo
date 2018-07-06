/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PROTOCOLSERIALIZER_H
#define EVEREST_BASE_PROTOCOLSERIALIZER_H

#include "Common/PluginProtocol/IProtocolSerializer.h"


namespace Common
{
    namespace PluginApiImpl
    {
        using data_t  = std::vector<std::string>;

        class ProtocolSerializer : public Common::PluginApi::IProtocolSerializer
        {
        public:
            ProtocolSerializer();
            const data_t serialize(const Common::PluginApi::DataMessage &dataMessage)const override;
            const Common::PluginApi::DataMessage deserialize(const data_t &serializedData) override;

        private:
            Common::PluginApi::DataMessage createDefaultErrorMessage();

            std::string m_supportedProtocolVersion;
        };
    }

}



#endif //EVEREST_BASE_PROTOCOLSERIALIZER_H
