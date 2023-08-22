/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>

namespace Common::ProtobufUtil
{
    class MessageException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
} // namespace Common::ProtobufUtil
