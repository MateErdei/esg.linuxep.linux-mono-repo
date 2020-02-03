/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../capability/PassOnCapability.h"

#include <unistd.h>

int main(int argc, char* argv[])
{
    // TODO get installation location
    int ret = pass_on_capability(CAP_DAC_READ_SEARCH);
    if (ret != 0)
    {
        return ret;
    }
    argv[0] = "scheduled_file_walker";
    execv("/opt/sophos-spl/plugins/av/sbin/scheduled_file_walker", argv);
    return 70; // If the exec fails
}
