/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GatherFiles.h"

#include <iostream>

namespace
{
    bool stringEndsWith(const std::string& str, const std::string& suffix)
    {
        return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
    }

    std::string getFileName(std::string string)
    {
        const size_t lastSlashPos = string.find_last_of('/');
        if (std::string::npos != lastSlashPos)
        {
            string.erase(0, lastSlashPos + 1);
        }
        else
        {
            throw std::invalid_argument("Filepath not a path: " + string);
        }
        return string;
    }
}

namespace diagnose
{
    void GatherFiles::setInstallDirectory(const Path& path)
    {
        m_installDirectory = path;
    }

    Path GatherFiles::createDiagnoseFolder(const Path& path)
    {

        if (m_fileSystem.isDirectory(path))
        {
            std::cout << "Directory path already exists: " << path << std::endl;
        }
        m_fileSystem.makedirs(path);

        if (m_fileSystem.isDirectory(path))
        {
            return path;
        }
        throw std::invalid_argument("Directory was not created");
    }

    void GatherFiles::copyLogFiles(const Path& destination)
    {
        const std::string configFileName = "DiagnoseLogFilePaths.conf";
        const Path configFilePath = getConfigLocation(configFileName);

        m_logFilePaths = getLogLocations(configFilePath);
        for (const auto& path: m_logFilePaths)
        {
            std::string filePath = Common::FileSystem::join(m_installDirectory, path);
            if (m_fileSystem.isFile(filePath))
            {
                std::cout <<  filePath << std::endl;
                std::cout <<  destination << std::endl;
                std::string filename = getFileName(filePath);
                m_fileSystem.copyFile(filePath,Common::FileSystem::join(destination, filename));
            }
            else
            {
                std::cout << "file not found " << filePath << std::endl;
            }
        }
    }

    void GatherFiles::copyMcsConfigFiles(const Path& destination)
    {
        const std::string configFileName = "DiagnoseMCSDirectoryPaths.conf";
        const Path configFilePath = getConfigLocation(configFileName);

        m_mcsConfigDirectories = getLogLocations(configFilePath);
        for (const auto& path: m_mcsConfigDirectories)
        {
            std::string dirPath = Common::FileSystem::join(m_installDirectory, path);
            if (m_fileSystem.isDirectory(dirPath))
            {
                std::vector<std::string> files = m_fileSystem.listFiles(dirPath);

                for(const auto& file : files)
                {

                    if (stringEndsWith(file,".XML") || stringEndsWith(file, ".xml"))
                    {
                        std::string filename = getFileName(file);
                        m_fileSystem.copyFile(file,Common::FileSystem::join(destination, filename));
                    }
                    else
                    {
                        std::cout << "Not XML file " << file << std::endl;
                    }

                }

            }
            else
            {
                std::cout << "Not a valid Directory " << dirPath << std::endl;
            }

        }
    }

    std::vector<std::string> GatherFiles::getLogLocations(const Path& inputFilePath)
    {
        std::vector<std::string> listOfLogPaths;

        if (m_fileSystem.isFile(inputFilePath))
        {
            return m_fileSystem.readLines(inputFilePath);
        }
        throw std::invalid_argument("Log locations config file does not exist: " + inputFilePath);
    }

    Path GatherFiles::getConfigLocation(const std::string& configFileName)
    {
        Path configFilePath = Common::FileSystem::join(m_fileSystem.currentWorkingDirectory(), configFileName);

        if (m_fileSystem.isFile(configFilePath))
        {
            std::cout << "Location of config file: " << configFilePath << std::endl;
        }
        else
        {
            configFilePath = Common::FileSystem::join(m_installDirectory, "base/etc", configFileName);
            if (m_fileSystem.isFile(configFilePath))
            {
                std::cout << "Location of config file: " << configFilePath << std::endl;
            }
            else
            {
                throw std::invalid_argument("No config file - " + configFileName);
            }
        }
        return configFilePath;
    }
}
