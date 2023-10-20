// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "products/capability/PassOnCapability.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    (void)argc;
    int ret = pass_on_capability(CAP_DAC_READ_SEARCH);
    if (ret != 0)
    {
        return ret;
    }
    set_no_new_privs();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    argv[0] = "scheduled_file_walker";

    char* installPath = getenv("SOPHOS_INSTALL");
    char* relativePath = "/plugins/av/bin/avscanner";
    char scannerPath[strlen(installPath) + strlen(relativePath) + 1];

    snprintf(scannerPath, sizeof(scannerPath), "%s%s", installPath, relativePath);
    execv(scannerPath, argv);
    return 70; // If the exec fails
}
