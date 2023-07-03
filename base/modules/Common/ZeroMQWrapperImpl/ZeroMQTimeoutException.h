// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/ZeroMQWrapper/IIPCTimeoutException.h"

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQTimeoutException : public Common::ZeroMQWrapper::IIPCTimeoutException
        {
        public:
            using Common::ZeroMQWrapper::IIPCTimeoutException::IIPCTimeoutException;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
