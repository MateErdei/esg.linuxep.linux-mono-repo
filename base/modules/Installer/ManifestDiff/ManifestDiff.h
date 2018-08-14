/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Datatypes/StringVector.h>
#include "Manifest.h"

namespace Installer
{
    namespace ManifestDiff
    {
        class ManifestDiff
        {
        public:
            static int manifestDiffMain(int argc, char* argv[]);
            static int manifestDiffMain(const Common::Datatypes::StringVector& argv);

            static void writeAdded(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest);

            static std::set<std::string> calculateAdded(const Manifest& oldManifest, const Manifest& newManifest);

            static PathSet entriesToPaths(const ManifestEntrySet& entries);
        };
    }
}
