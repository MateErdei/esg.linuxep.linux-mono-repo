/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Action.h"

#include <Common/ZeroMQWrapper/IContextSharedPtr.h>

namespace wdctl
{
    namespace wdctlactions
    {
        class ZMQAction : public Action
        {
        public:
            explicit ZMQAction(const wdctl::wdctlimpl::Arguments& args);
        protected:
            Common::ZeroMQWrapper::IContextSharedPtr m_context;
        };
    }
}


