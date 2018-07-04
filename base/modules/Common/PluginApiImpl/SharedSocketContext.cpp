/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SharedSocketContext.h"
namespace Common
{
    namespace PluginApiImpl
    {

        std::shared_ptr<Common::ZeroMQWrapper::IContext> sharedContext()
        {
            static std::weak_ptr<Common::ZeroMQWrapper::IContext> weakSharedContext;
            auto shared = weakSharedContext.lock();
            if ( shared )
            {
                return shared;
            }
            else
            {
                shared = Common::ZeroMQWrapper::createContext();
                weakSharedContext = shared;
                return shared;
            }
        }
    }
}