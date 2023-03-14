// Copyright 2023 Sophos All rights reserved.
#ifndef VERSIG_ARGUMENTS_H
#define VERSIG_ARGUMENTS_H

#include <string>

namespace manifest
{
    struct Arguments
    {
        std::string SignedFilepath; // Path to signed-file
        std::string CertsFilepath;  // Path to certificates-file
        std::string CRLFilepath;    // Path to CRL-file
        std::string DataDirpath;    // Path to directory containing data files
        bool fixDate = false;
        bool checkInstall = false;
        bool requireAllManifestFilesPresentOnDisk = false;
        bool requireAllDiskFilesPresentInManifest = false;
        bool requireSHA256 = true;
        bool allowSHA1signature = false;
    };
}

#endif // VERSIG_ARGUMENTS_H
