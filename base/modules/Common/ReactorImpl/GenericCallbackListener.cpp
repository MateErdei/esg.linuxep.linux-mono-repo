/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GenericCallbackListener.h"

namespace Common
{
    namespace Reactor
    {
        void GenericCallbackListener::process(std::vector<std::string> data)
        {
            m_callback(data);
        }

        GenericCallbackListener::GenericCallbackListener(GenericCallbackListener::CallbackFunction callback)
        {
            m_callback = callback;
        }
    }
}