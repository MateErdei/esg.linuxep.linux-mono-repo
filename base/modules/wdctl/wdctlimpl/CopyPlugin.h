/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include "Arguments.h"

namespace wdctl
{
    namespace wdctlimpl
    {
        class CopyPlugin
        {
        public:
            explicit CopyPlugin(const Arguments& args);
            int run();
        private:
            const Arguments& m_args;
        };
    }
}


