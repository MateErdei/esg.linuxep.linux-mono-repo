/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace EventTypes
    {
        class IEventException : public Common::Exceptions::IException
        {
        public:
            explicit IEventException(const std::string& what) : Common::Exceptions::IException(what) {}
        };
    } // namespace EventTypes
} // namespace Common
