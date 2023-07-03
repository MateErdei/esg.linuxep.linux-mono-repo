// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/ZeroMQWrapper/IIPCException.h"

#include <string>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQWrapperException : public Common::ZeroMQWrapper::IIPCException
        {
        public:
            using Common::ZeroMQWrapper::IIPCException::IIPCException;
        };

        class ZeroMQPollerException : public ZeroMQWrapperException
        {
        public:
            // inherits the constructor
            using ZeroMQWrapperException::ZeroMQWrapperException;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
