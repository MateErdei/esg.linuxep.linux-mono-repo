/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::OSUtilities
{
    class IPlatformUtilsException : public Common::Exceptions::IException
    {
    public:
        explicit IPlatformUtilsException(const std::string& what) : Common::Exceptions::IException(what) {}
    };
} // namespace Common::OSUtilities
