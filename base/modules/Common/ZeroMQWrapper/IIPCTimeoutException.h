// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IIPCException.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCTimeoutException : public IIPCException
        {
        public:
            using IIPCException::IIPCException;
        };
    } // namespace ZeroMQWrapper
} // namespace Common
