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

using Installer::VersionedCopy::VersionedCopy;

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

static Path getLinkDestination(const Path& link)
{
    const int BUFSIZE=1024;
    char dest[BUFSIZE];
    ssize_t size = ::readlink(link.c_str(), dest, BUFSIZE);
    if (size < 0)
    {
        return Path();
    }
    if (size == BUFSIZE)
    {
        // Overflow
        dest[BUFSIZE-1] = 0;
    }
    else
    {
        dest[size] = 0;
    }
    return Path(dest);
}

/**
 * Get a digit block from the end of s.
 * So Foo.14 returns 14
 * So Foo.1 returns 1
 * @param s
 * @return int
 */
int VersionedCopy::getDigitFromEnd(const std::string& s)
{
    size_t last_index = s.find_last_not_of("0123456789");
    std::string result = s.substr(last_index + 1);
    return std::stoi(result);
}

static Path findAppropriateExtensionName(const Path& base)
{
    // if fullInstallFilename exists, read symlink+1
    // otherwise fullInstallFilename.0
    Path current = getLinkDestination(base);
    int newDigit = 0;
    if (!current.empty())
    {
        int currentDigit = VersionedCopy::getDigitFromEnd(current);
        newDigit = currentDigit + 1;
    }

    std::ostringstream ost;
    ost << base << "." << newDigit;
    Path target = ost.str();
    return target;
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

static void createSymbolicLink(const Path& target, const Path& destination)
{
    Path tempDest = destination+".tmp";
    int ret = symlink(target.c_str(),tempDest.c_str());
    if (ret == 0)
    {
        rename(tempDest.c_str(), destination.c_str());
    }
}

static void createLibrarySymlinks(const Path& fullInstallFilename)
{
    Path basename = GL_filesystem->basename(fullInstallFilename);
    Path installDirname = GL_filesystem->dirName(fullInstallFilename);
    Path temp = basename;
    while (true)
    {
        size_t last_dot = temp.find_last_of('.');
        if (last_dot == Path::npos)
        {
            break;
        }
        Path dest = temp.substr(0,last_dot);
        Path fullDest = GL_filesystem->join(installDirname, dest);
        Path target = temp;

        std::string digits = temp.substr(last_dot + 1);
        if (digits.find_first_not_of("0123456789") != Path::npos)
        {
            break;
        }
        // digits is just numbers so continue
//        PRINT(fullDest << " -> " << target << "(digits="<<digits<<")");

        createSymbolicLink(target, fullDest);


        temp = dest;
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
