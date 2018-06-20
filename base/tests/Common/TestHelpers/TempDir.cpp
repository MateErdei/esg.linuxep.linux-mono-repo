///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "TempDir.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/FileSystem/IFileSystemException.h"
#include <cstring>
#include <sys/stat.h>

namespace Tests
{

    std::unique_ptr<TempDir> TempDir::makeTempDir(const std::string & nameprefix)
    {
        return std::unique_ptr<TempDir>( new TempDir("/tmp", nameprefix));
    }

    TempDir::TempDir(const std::string &baseDirectory, const std::string &namePrefix)
    {
        m_fileSystem = std::unique_ptr<Common::FileSystem::IFileSystem>(new Common::FileSystem::FileSystemImpl());
        std::string template_name;
        if ( baseDirectory == "" )
        {
            template_name = m_fileSystem->currentWorkingDirectory();
        }
        else
        {
            template_name = baseDirectory;
        }
        template_name = m_fileSystem->join(template_name, namePrefix);
        template_name += "XXXXXX";
        std::vector<char> aux_array(template_name.begin(), template_name.end());
        aux_array.emplace_back('\0');

        char * ptr2data = ::mkdtemp(aux_array.data());
        if ( ptr2data == nullptr)
        {
            int errn = errno;
            std::string error_cause = ::strerror(errn);
            throw Common::FileSystem::IFileSystemException("Failed to create directory: "+ template_name + ". Cause: " + error_cause);
        }
        m_rootpath = std::string(ptr2data);
    }

    TempDir::~TempDir()
    {
        m_fileSystem.reset();
        // clear the temporary directory recursively
        std::string quoted_command = "rm -rf \"" + m_rootpath + "\"";
        ::system(quoted_command.c_str());

    }

    std::string TempDir::dirPath() const
    {
        return m_rootpath;
    }

    std::string TempDir::absPath(const std::string& relativePath) const
    {
        return m_fileSystem->join(m_rootpath, relativePath);
    }

    void TempDir::makeDirs(const std::string& relativePath) const
    {
        auto parts = pathParts(relativePath);
        std::string dirpath = m_rootpath;
        for ( auto & dirname : parts)
        {
            dirpath = m_fileSystem->join(dirpath, dirname);
            if( !m_fileSystem->isDirectory(dirpath))
            {
                if ( ::mkdir( dirpath.c_str(), 0700) != 0)
                {
                    int errn = errno;
                    std::string error_cause = ::strerror(errn);
                    throw Common::FileSystem::IFileSystemException("Failed to create directory: "+ dirpath  + ". Cause: " + error_cause);
                }
            }
        }

    }

    void TempDir::makeDirs(const std::vector<std::string>& relativePaths) const
    {
        for( auto & dirname: relativePaths)
        {
            makeDirs(dirname);
        }
    }

    void TempDir::createFile(const std::string& relativePath, const std::string& content) const
    {
        std::string abs_path = absPath(relativePath);
        std::string filename = m_fileSystem->basename(abs_path);
        std::string dir_path = abs_path.substr(m_rootpath.size(), abs_path.size() - filename.size() - m_rootpath.size());
        makeDirs(dir_path);
        m_fileSystem->writeFile(abs_path, content);
    }

    std::vector<std::string> TempDir::pathParts(const std::string &relativePath)
    {
        std::vector<std::string> parts;
        int p_pos = 0;
        int c_pos = 0;
        while( c_pos < relativePath.size())
        {
            std::string dirname;
            c_pos = relativePath.find('/', p_pos);
            if ( c_pos == std::string::npos)
            {
                dirname =  relativePath.substr(p_pos, c_pos);
            }
            else
            {
                dirname =  relativePath.substr(p_pos, c_pos-p_pos);
            }

            if ( !dirname.empty())
            {
                parts.emplace_back(dirname);
            }


            p_pos = c_pos + 1;
        }

        return parts;
    }

    std::string TempDir::fileContent(const std::string &relativePath) const
    {
        return m_fileSystem->readFile(absPath(relativePath));

    }


}