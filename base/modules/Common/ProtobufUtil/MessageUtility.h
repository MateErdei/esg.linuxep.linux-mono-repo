/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace google
{
    namespace protobuf
    {
        class Message;
    }
} // namespace google
namespace Common
{
    namespace ProtobufUtil
    {
        class MessageUtility
        {
        public:
            static std::string protoBuf2Json(const google::protobuf::Message& message);
        };

    } // namespace ProtobufUtil
} // namespace Common
