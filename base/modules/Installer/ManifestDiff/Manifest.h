/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ManifestEntry.h"

#include <vector>

namespace Installer
{
    namespace ManifestDiff
    {
        /**
         * Represent a manifest file.
         */
        class Manifest
        {
        public:
            Manifest() = default;
            explicit Manifest(std::istream& file);
            static Manifest ManifestFromPath(const std::string& filepath);
            unsigned long size() const;
            ManifestEntrySet entries() const;
            PathSet paths() const;

            /**
             * Calculate entries that have been added into this manifest,
             * that didn't exist at all in the old manifest.
             * @param oldManifest
             * @return
             */
            ManifestEntrySet calculateAdded(const Manifest& oldManifest) const;

        private:
            ManifestEntryVector m_entries;
        };
    }
}


