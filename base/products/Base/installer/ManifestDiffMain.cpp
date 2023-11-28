// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"
#include "Installer/ManifestDiff/ManifestDiff.h"

static int inner_main(int argc, char* argv[])
{
    return Installer::ManifestDiff::manifestDiffMain(argc, argv);
}

MAIN(inner_main(argc, argv))