/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../capability/PassOnCapability.h"

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

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
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    char *envp[] =
            {
                    "LD_LIBRARY_PATH=/opt/sophos-spl/plugins/av/chroot/susi/distribution_version/:/opt/sophos-spl/plugins/av/chroot/susi/distribution_version/version1/",
                    0
            };

    argv[0] = "sophos_threat_detector";
    execve("/opt/sophos-spl/plugins/av/sbin/sophos_threat_detector", argv, envp);
    return 70; // If the exec fails
}
