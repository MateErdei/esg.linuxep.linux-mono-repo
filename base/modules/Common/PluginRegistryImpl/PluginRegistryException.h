/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef COMMON_PLUGINREGISTRYIMPL_PLUGINREGISTRYEXCEPTION_H
#define COMMON_PLUGINREGISTRYIMPL_PLUGINREGISTRYEXCEPTION_H
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
#endif //COMMON_PLUGINREGISTRYIMPL_PLUGINREGISTRYEXCEPTION_H
