/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/Exceptions/IException.h"

namespace Common
{
    namespace Obfuscation
    {
        class IObfuscationException : public Common::Exceptions::IException
        {
        public:
            explicit IObfuscationException(const std::string& what) : Common::Exceptions::IException(what) {}
        };
    } // namespace Obfuscation
} // namespace Common
