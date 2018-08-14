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

    return 0;
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
    std::ostringstream ost(destination);
    for (auto& a : added)
    {
        ost << a << std::endl;
    }
    Common::FileSystem::fileSystem()->writeFile(destination,ost.str());
}

PathSet ManifestDiff::calculateAdded(const Manifest& oldManifest, const Manifest& newManifest)
{
    return entriesToPaths(newManifest.calculateAdded(oldManifest));
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
