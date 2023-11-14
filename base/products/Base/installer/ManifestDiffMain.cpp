// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Installer/ManifestDiff/ManifestDiff.h"

#include "Common/Main/Main.h"

static int inner_main(int argc, char* argv[])
{
    return Installer::ManifestDiff::ManifestDiff::manifestDiffMain(argc, argv);
}

MAIN(inner_main(argc, argv))