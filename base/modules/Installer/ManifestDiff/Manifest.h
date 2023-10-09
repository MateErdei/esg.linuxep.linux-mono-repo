/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ManifestEntry.h"

#include <map>

namespace Installer::ManifestDiff
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

        /**
         * Calculate entries that have been removed from this manifest,
         * compared to the old manifest.
         * @param oldManifest
         * @return
         */
        ManifestEntrySet calculateRemoved(const Manifest& oldManifest) const;

        /**
         * Work out which entries have changed, that is they are present in both
         * the old and new manifests but have change length or hash.
         * @param oldManifest
         * @return
         */
        ManifestEntrySet calculateChanged(const Manifest& oldManifest) const;

    private:
        using ManifestEntryMap = std::map<std::string, ManifestEntry>;

        ManifestEntryMap getEntriesByPath() const;

        ManifestEntryVector m_entries;
    };
} // namespace Installer::ManifestDiff
