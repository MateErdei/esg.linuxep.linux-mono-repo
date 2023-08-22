/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace google::protobuf
{
    class Message;
} // namespace google::protobuf
namespace Common::ProtobufUtil
{
    class MessageUtility
    {
    public:
        static std::string protoBuf2Json(const google::protobuf::Message& message);
    };
} // namespace Common::ProtobufUtil
