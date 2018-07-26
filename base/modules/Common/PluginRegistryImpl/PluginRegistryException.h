/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include "Exceptions/IException.h"

namespace Common
{
    namespace PluginRegistryImpl
    {
        class PluginRegistryException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
        };
    }
}

