/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCException : public Common::Exceptions::IException
        {
        public:
            explicit IIPCException(const std::string& what) : Common::Exceptions::IException(what) {}
        };
    } // namespace ZeroMQWrapper
} // namespace Common
