// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "../capability/PassOnCapability.h"

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

static int pass_on_chroot_capability()
{
    return pass_on_capability(CAP_SYS_CHROOT);
}

#define SOPHOS_INSTALL "/opt/sophos-spl"
static const char* SOPHOS_THREAT_DETECTOR= SOPHOS_INSTALL "/plugins/av/sbin/sophos_threat_detector";

int main(int argc, char* argv[])
{
    (void)argc;
    int ret = pass_on_chroot_capability();
    if (ret != 0)
    {
        return ret;
    }
    set_no_new_privs();
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    // set LOCALDOMAIN to prevent getaddrinfo() from using nscd.
    char *envp[] =
            {
                    "LD_LIBRARY_PATH=" SOPHOS_INSTALL "/plugins/av/chroot/susi/distribution_version:" SOPHOS_INSTALL "/plugins/av/lib64",
                    "LOCALDOMAIN=",
                    0
            };

    argv[0] = (char*)SOPHOS_THREAT_DETECTOR;
    execve(SOPHOS_THREAT_DETECTOR, argv, envp);
    return 70; // If the exec fails
}
