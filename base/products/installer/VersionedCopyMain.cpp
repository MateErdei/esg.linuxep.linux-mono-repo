///////////////////////////////////////////////////////////
//
// Copyright (C) 2021 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <Installer/VersionedCopy/VersionedCopy.h>
#include <sys/stat.h>
int main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO); // Read and write for the owner
    return Installer::VersionedCopy::VersionedCopy::versionedCopyMain(argc, argv);
}
