/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManifestDiff.h"
#include "CommandLineOptions.h"
#include "Manifest.h"

#include <Common/FileSystem/IFileSystem.h>

#include <cassert>
#include <fstream>
#include <sstream>

using namespace Installer::ManifestDiff;

int ManifestDiff::manifestDiffMain(int argc, char** argv)
{
    Common::Datatypes::StringVector argvv;

    assert(argc>=1);
    for(int i=0; i<argc; i++)
    {
        argvv.emplace_back(argv[i]);
    }

    return manifestDiffMain(argvv);
}

int ManifestDiff::manifestDiffMain(const Common::Datatypes::StringVector& argv)
{
    CommandLineOptions options(argv);

    Manifest oldManifest(Manifest::ManifestFromPath(options.m_old));
    Manifest newManifest(Manifest::ManifestFromPath(options.m_new));

    writeAdded(options.m_added, oldManifest, newManifest);
    writeRemoved(options.m_removed, oldManifest, newManifest);

    return 0;
}

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
    Common::FileSystem::fileSystem()->writeFile(destination,toString(paths));
}

void ManifestDiff::writeAdded(const std::string& destination,
                              const Manifest& oldManifest,
                              const Manifest& newManifest)
{
    if (destination.empty())
    {
        return;
    }
    PathSet added = calculateAdded(oldManifest, newManifest);
    writeFile(destination,added);
}

PathSet ManifestDiff::calculateAdded(const Manifest& oldManifest, const Manifest& newManifest)
{
    return entriesToPaths(newManifest.calculateAdded(oldManifest));
}

void ManifestDiff::writeRemoved(const std::string& destination, const Manifest& oldManifest, const Manifest& newManifest)
{
    if (destination.empty())
    {
        return;
    }
    PathSet removed = entriesToPaths(newManifest.calculateRemoved(oldManifest));
    writeFile(destination,removed);
}
