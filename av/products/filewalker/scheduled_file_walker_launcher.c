/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../capability/PassOnCapability.h"

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

int main(int argc, char* argv[])
{
    // TODO get installation location LINUXDAR-2158
    int ret = pass_on_capability(CAP_DAC_READ_SEARCH);
    if (ret != 0)
    {
        return ret;
    }
    set_no_new_privs();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    argv[0] = "scheduled_file_walker";
    execv("/opt/sophos-spl/plugins/av/bin/avscanner", argv);
    return 70; // If the exec fails
}
