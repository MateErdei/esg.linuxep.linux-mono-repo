/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ReactorImpl.h"

namespace Common
{
    namespace Reactor
    {
        void ReactorImpl::registerListener( Common::ZeroMQWrapper::IReadable* , ICallbackListener * )
        {

        }

        void ReactorImpl::armShutdownListener(IShutdownListener *)
        {

        }
    }
}
