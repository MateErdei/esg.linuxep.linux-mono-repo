// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
        };
    } // namespace ZeroMQWrapper
} // namespace Common
