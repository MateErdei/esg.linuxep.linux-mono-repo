/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IObfuscationException.h"

namespace Common
{
    namespace Obfuscation
    {

        class IObscurityException: public IObfuscationException
        {
        public:
            explicit IObscurityException(const std::string& what)
                    : IObfuscationException(what)
            {}
        };
    }
}
