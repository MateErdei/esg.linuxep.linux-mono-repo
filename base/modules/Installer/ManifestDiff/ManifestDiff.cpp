// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ManifestDiff.h"

#include "CommandLineOptions.h"
#include "Manifest.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"

#include <cassert>
#include <fstream>
#include <sstream>

using namespace Installer::ManifestDiff;

int Installer::ManifestDiff::manifestDiffMain(int argc, char** argv)
{
    Common::Datatypes::StringVector argvv;

    assert(argc >= 1);
    for (int i = 0; i < argc; i++)
    {
        argvv.emplace_back(argv[i]);
    }

    Common::FileSystem::FileSystemImpl fileSystem;
    return manifestDiffMain(fileSystem, argvv);
}

int Installer::ManifestDiff::manifestDiffMain(
    Common::FileSystem::IFileSystem& fileSystem,
    const Common::Datatypes::StringVector& argv)
{
    CommandLineOptions options(argv);

    ManifestDiff manifestDiff{ fileSystem };

    Manifest oldManifest(Manifest::ManifestFromPath(fileSystem, options.m_old));
    Manifest newManifest(Manifest::ManifestFromPath(fileSystem, options.m_new));

    manifestDiff.writeAdded(options.m_added, oldManifest, newManifest);
    manifestDiff.writeRemoved(options.m_removed, oldManifest, newManifest);
    manifestDiff.writeChanged(options.m_changed, oldManifest, newManifest);

    return 0;
}

ManifestDiff::ManifestDiff(Common::FileSystem::IFileSystem& fileSystem) : fileSystem_{ fileSystem } {}

std::string ManifestDiff::toString(const PathSet& paths)
{
    std::ostringstream ost;
    for (auto& p : paths)
    {
        ost << p << std::endl;
    }
    return ost.str();
}

PathSet ManifestDiff::entriesToPaths(const ManifestEntrySet& entries)
{
    PathSet paths;
    for (const auto& entry : entries)
    {
        paths.insert(entry.path());
    }
    return paths;
}

void ManifestDiff::writeFile(const std::string& destination, const PathSet& paths)
{
    assert(!destination.empty());
    fileSystem_.makedirs(Common::FileSystem::dirName(destination));
    fileSystem_.writeFile(destination, toString(paths));
}

void ManifestDiff::writeAdded(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest)
{
    if (destination.empty())
    {
        return;
    }
    PathSet added = calculateAdded(oldManifest, newManifest);
    writeFile(destination, added);
}

PathSet ManifestDiff::calculateAdded(const Manifest& oldManifest, const Manifest& newManifest)
{
    return entriesToPaths(newManifest.calculateAdded(oldManifest));
}

void ManifestDiff::writeRemoved(
    const std::string& destination,
    const Manifest& oldManifest,
    const Manifest& newManifest)
{
    if (destination.empty())
    {
        return;
    }
    PathSet removed = entriesToPaths(newManifest.calculateRemoved(oldManifest));
    writeFile(destination, removed);
}

void ManifestDiff::writeChanged(
    const std::string& destination,
    const Manifest& oldManifest,
    const Manifest& newManifest)
{
    if (destination.empty())
    {
        return;
    }
    PathSet changed = entriesToPaths(newManifest.calculateChanged(oldManifest));
    writeFile(destination, changed);
}
