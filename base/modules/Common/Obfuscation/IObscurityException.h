/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IObfuscationException.h"

namespace Common::Obfuscation
{
    class IObscurityException : public IObfuscationException
    {
    public:
        explicit IObscurityException(const std::string& what) : IObfuscationException(what) {}
    };
} // namespace Common::Obfuscation
