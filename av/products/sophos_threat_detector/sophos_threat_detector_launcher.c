// Copyright 2020-2024 Sophos Limited. All rights reserved.

#include "common/ErrorCodesC.h"
#include "products/capability/PassOnCapability.h"
#include "sophos_install/SophosInstall.h"

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

static int pass_on_chroot_capability()
{
    return pass_on_capability(CAP_SYS_CHROOT);
}

int main(int argc, char* argv[])
{
    (void) argc;
    int ret = pass_on_chroot_capability();
    if (ret != 0) {
        return ret;
    }
    set_no_new_privs();
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    char installPath[PATH_MAX];
    ssize_t installPathSize = getSophosInstall(installPath, sizeof(installPath));
    if (installPathSize < 0)
    {
        return E_SOPHOS_INSTALL_NO_SET;
    }

    char* ldPathPrefix = "LD_LIBRARY_PATH=";
    char* susiPath = "/plugins/av/chroot/susi/distribution_version:";
    char* libPath = "/plugins/av/lib64";
    char ldLibraryPath[strlen(ldPathPrefix) + (2 * strlen(installPath)) + strlen(susiPath) + strlen(libPath) + 1];

    snprintf(ldLibraryPath, sizeof(ldLibraryPath), "%s%s%s%s%s",ldPathPrefix, installPath, susiPath, installPath, libPath);
    // set LOCALDOMAIN to prevent getaddrinfo() from using nscd.
    char* envp[] = { ldLibraryPath, "LOCALDOMAIN=", 0 };

    char* relativeThreatDetectorPath = "/plugins/av/sbin/sophos_threat_detector";
    char threatDetectorPath[strlen(installPath) + strlen(relativeThreatDetectorPath) + 1];
    snprintf(threatDetectorPath, sizeof(threatDetectorPath), "%s%s", installPath, relativeThreatDetectorPath);

    argv[0] = (char*)threatDetectorPath;
    execve(threatDetectorPath, argv, envp);
    return 70; // If the exec fails
}
