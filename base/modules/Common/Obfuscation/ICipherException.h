/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IObfuscationException.h"

namespace Common
{
    namespace Obfuscation
    {
        class ICipherException : public IObfuscationException
        {
        public:
            explicit ICipherException(const std::string& what) : IObfuscationException(what) {}
        };
    } // namespace Obfuscation
} // namespace Common
