// Copyright 2023 Sophos All rights reserved.

#include "Susi.h"

#include "stdio.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        return 2;
    }
    const char* installSourceFolder = "/opt/sophos-spl/plugins/av/chroot/susi/update_source";
    const char* installDestination = argv[1];
    SusiResult ret = SUSI_Install(installSourceFolder, installDestination);

    fprintf(stderr, "Result: %x\n", ret);
    fprintf(stderr, "Failure: %d\n", SUSI_FAILURE(ret));
    return 0;
}
