// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Installer/VersionedCopy/VersionedCopy.h"

#include <sys/stat.h>

int main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO); // Read and write for the owner
    return Installer::VersionedCopy::VersionedCopy::versionedCopyMain(argc, argv);
}
