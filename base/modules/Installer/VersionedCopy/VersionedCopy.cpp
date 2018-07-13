///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "VersionedCopy.h"

#include <Common/Exceptions/Print.h>
#include <Common/FileSystem/IFileSystem.h>

#include <cassert>
#include <string>
#include <vector>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

using Installer::VersionedCopy::VersionedCopy;

static Common::FileSystem::IFileSystemPtr GL_filesystem;

namespace
{
    Common::FileSystem::IFileSystem* fs()
    {
        if (GL_filesystem.get() == nullptr)
        {
            GL_filesystem = Common::FileSystem::createFileSystem();
        }
        return GL_filesystem.get();
    }

    std::string getEnv(const std::string &variable, const std::string &defaultValue)
    {
        const char *value = ::secure_getenv(variable.c_str());
        if (value == nullptr)
        {
            return defaultValue;
        }
        return value;
    }

    std::string getInstallationRelativeFilename(const std::string &filename, const std::string &DIST)
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

    Path getDirname(const Path &path)
    {
        return fs()->dirName(path);
    }

    void makedirs(const Path &origpath)
    {
        fs()->makedirs(origpath);
    }

    Path getLinkDestination(const Path &link)
    {
        const int BUFSIZE = 1024;
        char dest[BUFSIZE];
        ssize_t size = ::readlink(link.c_str(), dest, BUFSIZE);
        if (size < 0)
        {
            return Path();
        }
        if (size == BUFSIZE)
        {
            // Overflow
            dest[BUFSIZE - 1] = 0;
        } else
        {
            dest[size] = 0;
        }
        return Path(dest);
    }

    Path findAppropriateExtensionName(const Path &base)
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

    void deleteOldFiles(const Path &filename, const Path &newFile)
    {
        if (newFile.find(filename) != 0)
        {
            PRINT("deleteOldFiles with invalid input: "<<filename<<", "<< newFile);
            return;
        }

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
            if (fs()->exists(target))
            {
                unlink(target.c_str());
            }
            i++;
        }
    }

    void copyFile(const Path &src, const Path &dest)
    {
        fs()->copyFile(src,dest);
    }

    void createSymbolicLink(const Path &target, const Path &destination)
    {
        Path tempDest = destination + ".tmp";
        int ret = symlink(target.c_str(), tempDest.c_str());
        if (ret == 0)
        {
            rename(tempDest.c_str(), destination.c_str());
        }
    }

    void createLibrarySymlinks(const Path &fullInstallFilename)
    {
        Path basename = fs()->basename(fullInstallFilename);
        Path installDirname = fs()->dirName(fullInstallFilename);
        Path temp = basename;
        while (true)
        {
            size_t last_dot = temp.find_last_of('.');
            if (last_dot == Path::npos)
            {
                break;
            }
            Path dest = temp.substr(0, last_dot);
            Path fullDest = fs()->join(installDirname, dest);
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

    int versionedCopyVector(const std::vector<std::string> &argv)
    {
        const std::string DIST = getEnv("DIST", ".");
        const std::string INST = getEnv("SOPHOS_INSTALL", "/opt/sophos-spl");

        std::string filename = argv.at(1);
        return Installer::VersionedCopy::VersionedCopy::versionedCopy(filename, DIST, INST);
    }
}

/**
 * Get a digit block from the end of s.
 * So Foo.14 returns 14
 * So Foo.1 returns 1
 * @param s
 * @return int
 */
int VersionedCopy::getDigitFromEnd(const std::string &s)
{
    if (s.empty())
    {
        return -1;
    }
    size_t last_index = s.find_last_not_of("0123456789");
    if (last_index == std::string::npos)
    {
        // All numbers
        return std::stoi(s);
    }
    else if (last_index == s.size()-1)
    {
        // No number at end
        return -1;
    }
    std::string result = s.substr(last_index + 1);
    return std::stoi(result);
}

bool VersionedCopy::same(const Path& file1, const Path& file2)
{
    bool exists1 = fs()->exists(file1);
    bool exists2 = fs()->exists(file2);
    if (exists1 != exists2)
    {
        return false;
    }
    if (!exists1)
    {
        assert(!exists2);
        return true;
    }

    std::ifstream f1(file1, std::ifstream::binary|std::ifstream::ate);
    std::ifstream f2(file2, std::ifstream::binary|std::ifstream::ate);

    if (f1.fail() || f2.fail())
    {
        return false; // failed to open file
    }

    if (f1.tellg() != f2.tellg())
    {
        return false; // size mismatch
    }

    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);

    const int BLOCK_SIZE=1024;
    while (f1.good() && f2.good())
    {
        char data1[BLOCK_SIZE];
        char data2[BLOCK_SIZE];

        f1.read(data1,BLOCK_SIZE);
        f2.read(data2,BLOCK_SIZE);

        // Files are different lengths
        if (f1.gcount() != f2.gcount())
        {
            return false;
        }
        // One has hit EOF, and the other hasn't
        if (f1.good() != f2.good())
        {
            return false;
        }

        if (::memcmp(data1,data2,static_cast<size_t>(f1.gcount())) != 0)
        {
            return false;
        }
    }
    return true;
}

int VersionedCopy::versionedCopy(const Path &filename, const Path &DIST, const Path &INST)
{
    if (!fs()->exists(filename))
    {
        return 1;
    }

    std::string installationFilename = getInstallationRelativeFilename(filename, DIST);

    std::string fullInstallFilename = INST + "/" + installationFilename;


    // Don't continue if the files are the same
    if (same(filename, fullInstallFilename))
    {
        return 0;
    }

    std::string installationDir = getDirname(fullInstallFilename);
    makedirs(installationDir); // TODO: Permissions and ownership

    // Find appropriate extension name
    Path extensionName = findAppropriateExtensionName(fullInstallFilename);
    Path extensionBase = fs()->basename(extensionName);

    // Copy file
    copyFile(filename, extensionName); // TODO: Permissions and ownership

    // Change/create symlink
    createSymbolicLink(extensionBase, fullInstallFilename);

    // Delete old files (ignore failure)
    deleteOldFiles(fullInstallFilename, extensionName);

    // Create library symlinks
    createLibrarySymlinks(fullInstallFilename);

    return 0;
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
