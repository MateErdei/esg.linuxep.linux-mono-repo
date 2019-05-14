/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace UtilityImpl
    {
        class MessageException : public Common::Exceptions::IException
        {
        public:
            explicit MessageException(const std::string& what) : Common::Exceptions::IException(what) {}
        };

    } // namespace UtilityImpl
} // namespace Common
