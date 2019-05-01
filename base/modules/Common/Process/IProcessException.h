/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace Process
    {
        class IProcessException : public Common::Exceptions::IException
        {
        public:
            explicit IProcessException(const std::string& what) : Common::Exceptions::IException(what) {}
        };
    } // namespace Process
} // namespace Common
