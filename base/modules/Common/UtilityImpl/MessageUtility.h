/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}
namespace Common
{
    namespace UtilityImpl
    {
        class MessageUtility
        {
        public:
            static std::string protoBuf2Json(const google::protobuf::Message &message);
        };

    }
}

