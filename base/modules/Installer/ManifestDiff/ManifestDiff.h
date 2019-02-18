/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Manifest.h"

#include <Common/Datatypes/StringVector.h>

namespace Installer
{
    namespace ManifestDiff
    {
        class ManifestDiff
        {
        public:
            static int manifestDiffMain(int argc, char* argv[]);
            static int manifestDiffMain(const Common::Datatypes::StringVector& argv);

            static PathSet entriesToPaths(const ManifestEntrySet& entries);
            static std::string toString(const PathSet& paths);
            static void writeFile(const std::string& destination, const PathSet& paths);

            static void writeAdded(
                const std::string& destination,
                const Manifest& oldManifest,
                const Manifest& newManifest);
            static std::set<std::string> calculateAdded(const Manifest& oldManifest, const Manifest& newManifest);

            static void writeRemoved(
                const std::string& destination,
                const Manifest& oldManifest,
                const Manifest& newManifest);
            static void writeChanged(
                const std::string& destination,
                const Manifest& oldManifest,
                const Manifest& newManifest);
        };
    } // namespace ManifestDiff
} // namespace Installer
