// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Manifest.h"

#include "Common/Datatypes/StringVector.h"
#include "Common/FileSystem/IFileSystem.h"

namespace Installer::ManifestDiff
{
    int manifestDiffMain(int argc, char* argv[]);
    int manifestDiffMain(Common::FileSystem::IFileSystem& fileSystem, const Common::Datatypes::StringVector& argv);

    class ManifestDiff
    {
    public:
        explicit ManifestDiff(Common::FileSystem::IFileSystem& fileSystem);

        void writeAdded(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest);
        std::set<std::string> calculateAdded(const Manifest& oldManifest, const Manifest& newManifest);

        void writeRemoved(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest);
        void writeChanged(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest);

    private:
        PathSet entriesToPaths(const ManifestEntrySet& entries);
        std::string toString(const PathSet& paths);
        void writeFile(const std::string& destination, const PathSet& paths);

        Common::FileSystem::IFileSystem& fileSystem_;
    };
} // namespace Installer::ManifestDiff
