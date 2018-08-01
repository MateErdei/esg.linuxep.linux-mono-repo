/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>

namespace wdctl
{
    namespace wdctlimpl
    {
        class wdctl_bootstrap
        {
        public:
            using StringVector = std::vector<std::string>;
            int main(int argc, char* argv[]);
            int main(const StringVector& args);
            static StringVector convertArgv(unsigned int argc, char* argv[]);
        };
    }
}
