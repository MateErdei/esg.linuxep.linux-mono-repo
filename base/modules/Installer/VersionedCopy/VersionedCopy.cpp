///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "VersionedCopy.h"

#include <Common/Datatypes/SophosCppStandard.h>
#include <Common/Exceptions/Print.h>
#include <Common/FileSystem/IFileSystem.h>

#include <cassert>
#include <string>
#include <vector>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

static Common::FileSystem::IFileSystemPtr GL_filesystem;

static std::string getEnv(const std::string& variable, const std::string& defaultValue)
{
    const char* value = ::getenv(variable.c_str());
    if (value == NULLPTR)
    {
        return defaultValue;
    }
    return value;
}

static std::string getInstallationRelativeFilename(const std::string &filename, const std::string &DIST)
{
    std::string ret = filename;
    if (filename.find(DIST) == 0)
    {
        ret = filename.substr(DIST.length() + 1);
    }
    if (ret.find("files/") == 0)
    {
        ret = ret.substr(6);
    }

    return ret;
}

static Path getDirname(const Path& path)
{
    return GL_filesystem->dirName(path);
}

static void makedirs(const Path& origpath)
{
    GL_filesystem->makedirs(origpath);
}

static Path findAppropriateExtensionName(const Path& base)
{
    // if fullInstallFilename exists, read symlink+1
    // otherwise fullInstallFilename.0
    int i = 0;
    while (true)
    {
        std::ostringstream ost;
        ost << base << "." << i;
        Path target = ost.str();
        if (! GL_filesystem->exists(target))
        {
            return target;
        }
        i++;
    }
}

static void deleteOldFiles(const Path& filename, const Path& newFile)
{
    int i = 0;
    while (true)
    {
        std::ostringstream ost;
        ost << filename << "." << i;
        Path target = ost.str();
        if (target == newFile)
        {
            return;
        }
        if (GL_filesystem->exists(target))
        {
            unlink(target.c_str());
        }
        i++;
    }
}

static void copyFile(const Path& src, const Path& dest)
{
    std::ifstream ifs(src);
    std::ofstream ofs(dest);

    ofs << ifs.rdbuf();
}

static void createLibrarySymlinks(const Path& fullInstallFilename)
{
    // TODO: Create library symlinks
    static_cast<void>(fullInstallFilename);
}

static void createSymbolicLink(const Path& target, const Path& destination)
{
    Path tempDest = destination+".tmp";
    int ret = symlink(target.c_str(),tempDest.c_str());
    if (ret == 0)
    {
        rename(tempDest.c_str(), destination.c_str());
    }
}

int Installer::VersionedCopy::VersionedCopy::versionedCopy(const Path& filename, const Path& DIST, const Path& INST)
{
    GL_filesystem = Common::FileSystem::createFileSystem();

    std::string installationFilename = getInstallationRelativeFilename(filename, DIST);

    std::string fullInstallFilename = INST+"/"+installationFilename;

    std::string installationDir = getDirname(fullInstallFilename);
    makedirs(installationDir); // TODO: Permissions and ownership

    // Find appropriate extension name
    Path extensionName = findAppropriateExtensionName(fullInstallFilename);
    Path extensionBase = GL_filesystem->basename(extensionName);

    // Copy file
    copyFile(filename, extensionName); // TODO: Permissions and ownership

    // Change/create symlink
    createSymbolicLink(extensionBase,fullInstallFilename);

    // Delete old files (ignore failure)
    deleteOldFiles(fullInstallFilename, extensionName);

    // Create library symlinks
    createLibrarySymlinks(fullInstallFilename);

    return 0;
}

static int versionedCopyVector(const std::vector<std::string>& argv)
{
    const std::string DIST = getEnv("DIST",".");
    const std::string INST = getEnv("SOPHOS_INSTALL","/opt/sophos-spl");

    std::string filename = argv.at(1);
    return Installer::VersionedCopy::VersionedCopy::versionedCopy(filename, DIST, INST);
}

int Installer::VersionedCopy::VersionedCopy::versionedCopyMain(int argc, char **argv)
{
    std::vector<std::string> argvv;

    assert(argc>=1);
    for(int i=0; i<argc; i++)
    {
        argvv.emplace_back(argv[i]);
    }

    return versionedCopyVector(argvv);
}
