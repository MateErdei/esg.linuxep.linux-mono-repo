/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GatherFiles.h"

#include <iostream>
#include <algorithm>
#include <string>


namespace
{
    bool stringEndsWith(const std::string& str, const std::string& suffix)
    {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    bool isFileOfInterest(std::string filename)
    {
        // Transform copy of string to lowercase
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

        static const std::vector<std::string> interestingExtensions{".xml",
                                                                   ".json",
                                                                   ".txt",
                                                                   ".conf",
                                                                   ".config",
                                                                   ".log",
                                                                   ".log.1",
                                                                   ".log.2",
                                                                   ".log.3",
                                                                   ".log.4",
                                                                   ".log.5",
                                                                   ".log.6",
                                                                   ".log.7",
                                                                   ".log.8",
                                                                   ".log.9"};

        for(const auto& type : interestingExtensions)
        {
            if (stringEndsWith(filename,type))
            {
                return true;
            }
        }
        return false;
    }
} // namespace

namespace diagnose
{
    void GatherFiles::setInstallDirectory(const Path& path)
    {
        m_installDirectory = path;
    }

    Path GatherFiles::createDiagnoseFolder(const Path& path)
    {
        if (!m_fileSystem.isDirectory(path))
        {
            throw std::invalid_argument("Directory does not exist");
        }

        Path outputDir = Common::FileSystem::join(path, "DiagnoseOutput");
        if (m_fileSystem.isDirectory(outputDir))
        {
            throw std::invalid_argument("Output directory already exists: " + outputDir);
        }

        m_fileSystem.makedirs(outputDir);
        if (m_fileSystem.isDirectory(outputDir))
        {
            return outputDir;
        }

        throw std::invalid_argument("Directory was not created");
    }

    void GatherFiles::copyFile(const Path& filePath, const Path& destination)
    {
        std::cout << "Copied " << filePath << " to " << destination << std::endl;
        std::string filename = Common::FileSystem::basename(filePath);
        m_fileSystem.copyFile(filePath, Common::FileSystem::join(destination, filename));
    }

    void GatherFiles::copyAllOfInterestFromDir(const Path& dirPath, const Path& destination)
    {
        std::vector<std::string> files = m_fileSystem.listFiles(dirPath);
        for (const auto& file : files)
        {
            if (isFileOfInterest(file))
            {
                copyFile(file, destination);
            }
        }
    }

    void GatherFiles::copyBaseFiles(const Path& destination)
    {
        const Path configFilePath = getConfigLocation("DiagnosePaths.conf");

        m_logFilePaths = getLogLocations(configFilePath);
        for (const auto& path : m_logFilePaths)
        {
            if (path.empty())
            {
                continue;
            }

            std::string filePath = Common::FileSystem::join(m_installDirectory, path);
            if (m_fileSystem.isFile(filePath))
            {
                copyFile(filePath, destination);
            }
            else if (m_fileSystem.isDirectory(filePath))
            {
                copyAllOfInterestFromDir(filePath, destination);
            }
            else
            {
                std::cout << "Error " << filePath << " is neither a file nor directory." << std::endl;
            }
        }
    }

    std::vector<std::string> GatherFiles::getLogLocations(const Path& inputFilePath)
    {
        if (m_fileSystem.isFile(inputFilePath))
        {
            return m_fileSystem.readLines(inputFilePath);
        }
        throw std::invalid_argument("Log locations config file does not exist: " + inputFilePath);
    }

    Path GatherFiles::getConfigLocation(const std::string& configFileName)
    {
        Path configFilePath =
            Common::FileSystem::join(m_fileSystem.currentWorkingDirectory(), "../etc", configFileName);

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

    void GatherFiles::copyPluginFiles(const Path& destination)
    {
        // Find names of the plugins installed
        Path pluginsDir = Common::FileSystem::join(m_installDirectory, "plugins");
        if(!m_fileSystem.isDirectory(pluginsDir))
        {
            return;
        }
        std::vector<Path> pluginDirs = m_fileSystem.listDirectories(pluginsDir);

        for (const auto& absolutePluginPath : pluginDirs)
        {
            std::string pluginName = Common::FileSystem::basename(absolutePluginPath);

            Path pluginLogDir = Common::FileSystem::join(pluginsDir, pluginName, "log");
            if (m_fileSystem.isDirectory(pluginLogDir))
            {
                copyAllOfInterestFromDir(pluginLogDir, destination);
            }
        }
    }
} // namespace diagnose
