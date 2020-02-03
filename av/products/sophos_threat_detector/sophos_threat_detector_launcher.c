/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../capability/PassOnCapability.h"

#include <unistd.h>

static int pass_on_chroot_capability()
{
    return pass_on_capability(CAP_SYS_CHROOT);
}

int main(int argc, char* argv[])
{
    int ret = pass_on_chroot_capability();
    if (ret != 0)
    {
        return ret;
    }
    set_no_new_privs();
    argv[0] = "sophos_threat_detector";
    execv("/opt/sophos-spl/plugins/av/sbin/sophos_threat_detector", argv);
    return 70; // If the exec fails
}
