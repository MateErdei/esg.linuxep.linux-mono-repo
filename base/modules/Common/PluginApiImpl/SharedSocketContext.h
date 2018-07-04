/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_SHAREDSOCKETCONTEXT_H
#define EVEREST_BASE_SHAREDSOCKETCONTEXT_H
#include "Common/ZeroMQWrapper/IContext.h"
namespace Common
{
    namespace PluginApiImpl
    {
        std::shared_ptr<Common::ZeroMQWrapper::IContext> sharedContext();
    }
}


#endif //EVEREST_BASE_SHAREDSOCKETCONTEXT_H
