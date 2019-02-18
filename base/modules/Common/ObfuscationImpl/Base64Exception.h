/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IObfuscationException.h"

namespace Common
{
    namespace Obfuscation
    {
        class IBase64Exception : public IObfuscationException
        {
        public:
            explicit IBase64Exception(const std::string& what) : IObfuscationException(what) {}
        };
    } // namespace Obfuscation
} // namespace Common
