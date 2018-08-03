/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "wdctl/wdctlimpl/Arguments.h"

namespace wdctl
{
    namespace wdctlactions
    {
        class Action
        {
        public:
            explicit Action(const wdctl::wdctlimpl::Arguments& args);
            virtual int run() = 0;
        protected:
            const wdctl::wdctlimpl::Arguments& m_args;
        };
    }
}


