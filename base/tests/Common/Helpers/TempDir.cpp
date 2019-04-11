///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "TempDir.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/FileSystemImpl/TempDir.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
namespace Tests
{
    std::unique_ptr<TempDir> TempDir::makeTempDir(const std::string& nameprefix)
    {
        return std::unique_ptr<TempDir>(new TempDir("/tmp", nameprefix));
    }

    TempDir::TempDir(const std::string& baseDirectory, const std::string& namePrefix) :
        m_fileSystem(new Common::FileSystem::FileSystemImpl()),
        m_tempDirBase(new Common::FileSystemImpl::TempDir(baseDirectory, namePrefix))
    {

    }

    Path TempDir::dirPath() const
    {
        return m_tempDirBase->dirPath();
    }

    std::string TempDir::absPath(const std::string& relativePath) const
    {
        return Common::FileSystem::join(dirPath(), relativePath);
    }

    void TempDir::makeDirs(const std::string& relativePath) const
    {
        auto parts = pathParts(relativePath);
        std::string dirpath = dirPath();
        for (auto& dirname : parts)
        {
            dirpath = Common::FileSystem::join(dirpath, dirname);
            if (!m_fileSystem->isDirectory(dirpath))
            {
                if (::mkdir(dirpath.c_str(), 0700) != 0)
                {
                    int errn = errno;
                    std::string error_cause = ::strerror(errn);
                    throw Common::FileSystem::IFileSystemException(
                        "Failed to create directory: " + dirpath + ". Cause: " + error_cause);
                }
            }
        }
    }

    void TempDir::makeDirs(const std::vector<std::string>& relativePaths) const
    {
        for (auto& dirname : relativePaths)
        {
            makeDirs(dirname);
        }
    }

    void TempDir::createFile(const std::string& relativePath, const std::string& content) const
    {
        std::string abs_path = absPath(relativePath);
        std::string filename = Common::FileSystem::basename(abs_path);
        std::string dir_path =
            abs_path.substr(dirPath().size(), abs_path.size() - filename.size() - dirPath().size());
        makeDirs(dir_path);
        m_fileSystem->writeFile(abs_path, content);
    }
    void TempDir::createFileAtomically(const std::string& relativePath, const std::string& content) const
    {
        std::string abs_path = absPath(relativePath);
        std::string filename = Common::FileSystem::basename(abs_path);
        std::string dir_path =
            abs_path.substr(dirPath().size(), abs_path.size() - filename.size() - dirPath().size());
        makeDirs(dir_path);
        auto tempDir = TempDir::makeTempDir();
        m_fileSystem->writeFileAtomically(abs_path, content, tempDir->dirPath());
    }

    std::vector<std::string> TempDir::pathParts(const std::string& relativePath)
    {
        std::vector<std::string> parts;
        size_t p_pos = 0;
        size_t c_pos = 0;
        while (c_pos < relativePath.size())
        {
            std::string dirname;
            c_pos = relativePath.find('/', p_pos);
            if (c_pos == std::string::npos)
            {
                dirname = relativePath.substr(p_pos, c_pos);
            }
            else
            {
                dirname = relativePath.substr(p_pos, c_pos - p_pos);
            }

            if (!dirname.empty())
            {
                parts.emplace_back(dirname);
            }

            p_pos = c_pos + 1;
        }

        return parts;
    }

    std::string TempDir::fileContent(const std::string& relativePath) const
    {
        return m_fileSystem->readFile(absPath(relativePath));
    }

    void TempDir::makeExecutable(const std::string& relativePath) const
    {
        std::string path = absPath(relativePath);
        assert(m_fileSystem->exists(path));
        Common::FileSystem::filePermissions()->chmod(path.c_str(), 0700);
    }

    bool TempDir::exists(const std::string& relativePath) const
    {
        std::string path = absPath(relativePath);
        return m_fileSystem->exists(path);
    }

} // namespace Tests