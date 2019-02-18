/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ZeroMQWrapper/IIPCException.h>

#include <string>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQWrapperException : public Common::ZeroMQWrapper::IIPCException
        {
        public:
            explicit ZeroMQWrapperException(const std::string& message) : Common::ZeroMQWrapper::IIPCException(message)
            {
            }
        };

        class ZeroMQPollerException : public ZeroMQWrapperException
        {
        public:
            // inherits the constructor
            using ZeroMQWrapperException::ZeroMQWrapperException;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
