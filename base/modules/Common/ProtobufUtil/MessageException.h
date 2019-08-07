/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>

namespace Common
{
    namespace ProtobufUtil
    {
        class MessageException : public std::runtime_error
        {
        public:
            using std::runtime_error::runtime_error;
        };

    } // namespace ProtobufUtil
} // namespace Common
