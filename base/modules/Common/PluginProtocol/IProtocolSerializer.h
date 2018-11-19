/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/PluginProtocol/DataMessage.h"
#include "Common/ZeroMQWrapper/IDataType.h"

#include <string>
#include <vector>

namespace Common
{
    namespace PluginProtocol
    {
        using data_t =  Common::ZeroMQWrapper::data_t;

        class IProtocolSerializer
        {

        public:
            virtual ~IProtocolSerializer() = default;

            /**
             * Class used to abstract out the serialization and deserialization of messages based on message protocol
             *
             * @param data DataMessage to convert to a PluginProtocolProto::PluginAPIMessage
             * @return  PluginProtocolProto::PluginAPIMessage containing the data obtained from data message based on
             *          protocol specified in the data message.
             */
            virtual const data_t serialize(const DataMessage &data)const = 0;

            /**
             *
             * @param data to convert to a DataMessage object
             * @return Data Message containing the message obtained from data input value based on the protocol
             *          specified in the input data.
             */
            virtual const DataMessage deserialize(const data_t &data) = 0;
        };
    }

}

