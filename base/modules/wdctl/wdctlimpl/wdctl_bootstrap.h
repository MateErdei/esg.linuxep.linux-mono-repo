/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "StringVector.h"

#include "wdctl/wdctlarguments/Arguments.h"

namespace wdctl::wdctlimpl
{
    class wdctl_bootstrap
    {
    public:
        int main(int argc, char* argv[]);
        int main(const StringVector& args);
        int main_afterLogConfigured(const StringVector& args, bool detectWatchdog = true);
        static StringVector convertArgv(unsigned int argc, char* argv[]);

    private:
        wdctlarguments::Arguments m_args;
    };
} // namespace wdctl::wdctlimpl
