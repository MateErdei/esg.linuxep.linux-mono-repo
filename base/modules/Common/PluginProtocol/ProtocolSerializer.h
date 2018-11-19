/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/PluginProtocol/IProtocolSerializer.h"


namespace Common
{
    namespace PluginProtocol
    {
        using data_t  = std::vector<std::string>;

        class ProtocolSerializer : public Common::PluginProtocol::IProtocolSerializer
        {
        public:
            ProtocolSerializer() = default;
            const data_t serialize(const Common::PluginProtocol::DataMessage &dataMessage)const override;
            const Common::PluginProtocol::DataMessage deserialize(const data_t &serializedData) override;

        private:
            Common::PluginProtocol::DataMessage createDefaultErrorMessage();
        };
    }

}




