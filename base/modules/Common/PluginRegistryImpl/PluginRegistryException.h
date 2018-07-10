/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_PLUGINREGISTRYEXCEPTION_H
#define EVEREST_BASE_PLUGINREGISTRYEXCEPTION_H
#include "Common/Exceptions/IException.h"

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
#endif //EVEREST_BASE_PLUGINREGISTRYEXCEPTION_H
