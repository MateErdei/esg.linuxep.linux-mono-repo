/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GenericCallbackListener.h"

namespace Common
{
    namespace ReactorImpl
    {
        void GenericCallbackListener::process(Common::ZeroMQWrapper::IReadable::data_t data)
        {
            m_callback(data);
        }

        GenericCallbackListener::GenericCallbackListener(GenericCallbackListener::CallbackFunction callback)
        {
            if ( callback )
            {
                m_callback = callback;
            }
            else
            {
                m_callback = [](Common::ZeroMQWrapper::IReadable::data_t ){};
            }
        }
    }
}