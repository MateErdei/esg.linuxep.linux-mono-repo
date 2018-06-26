/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GenericCallbackListener.h"

namespace Common
{
    namespace Reactor
    {
        ProcessInstruction GenericCallbackListener::process(std::vector<std::string> data)
        {
            m_callback(data);
            return ProcessInstruction::CONTINUE;
        }

        GenericCallbackListener::GenericCallbackListener(GenericCallbackListener::CallbackFunction callback)
        {
            if ( callback )
            {
                m_callback = callback;
            }
            else
            {
                m_callback = [](std::vector<std::string> ){ return ProcessInstruction::CONTINUE;};
            }
        }
    }
}