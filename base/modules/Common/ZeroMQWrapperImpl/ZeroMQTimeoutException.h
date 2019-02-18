/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQTimeoutException : public Common::ZeroMQWrapper::IIPCTimeoutException
        {
        public:
            explicit ZeroMQTimeoutException(const std::string& message) :
                Common::ZeroMQWrapper::IIPCTimeoutException(message)
            {
            }
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
