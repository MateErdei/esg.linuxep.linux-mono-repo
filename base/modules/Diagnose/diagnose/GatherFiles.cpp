/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GatherFiles.h"

#include "Logger.h"
#include "Strings.h"


#include <Common/FileSystemImpl/TempDir.h>

#include <algorithm>
#include <iostream>

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

        static const std::vector<std::string> interestingExtensions{ ".xml", ".json",  ".txt", ".conf", ".config",
                                                                     ".log", ".log.1", ".dat", ".flags" };

        for (const auto& type : interestingExtensions)
        {
            if (stringEndsWith(filename, type))
            {
                return true;
            }
        }
        return false;
    }
} // namespace

namespace diagnose
{
    void GatherFiles::setInstallDirectory(const Path& path) { m_installDirectory = path; }

    Path GatherFiles::createRootDir(const Path& path)
    {
        m_tempDir.reset(new Common::FileSystemImpl::TempDir(path, DIAGNOSE_FOLDER));
        return createDiagnoseFolder(m_tempDir->dirPath(), DIAGNOSE_FOLDER);
    }

    Path GatherFiles::createBaseFilesDir(const Path& path) { return createDiagnoseFolder(path, BASE_FOLDER); }

    Path GatherFiles::createPluginFilesDir(const Path& path) { return createDiagnoseFolder(path, PLUGIN_FOLDER); }

    Path GatherFiles::createSystemFilesDir(const Path& path) { return createDiagnoseFolder(path, SYSTEM_FOLDER); }

    Path GatherFiles::createDiagnoseFolder(const Path& path, const std::string& dirName)
    {
        if (!m_fileSystem.isDirectory(path))
        {
            throw std::invalid_argument("Error: Directory does not exist");
        }

        Path outputDir = Common::FileSystem::join(path, dirName);
        if (m_fileSystem.isDirectory(outputDir))
        {
            if (dirName == DIAGNOSE_FOLDER)
            {
                LOGINFO("Diagnose folder already exists");
            }
            else
            {
                throw std::invalid_argument(
                    "Error: Previous execution of Diagnose tool has not cleaned up. Please remove " + outputDir);
            }
        }

        m_fileSystem.makedirs(outputDir);
        if (m_fileSystem.isDirectory(outputDir))
        {
            return outputDir;
        }

        throw std::invalid_argument("Error: Directory was not created");
    }

    void GatherFiles::copyFileIntoDirectory(const Path& filePath, const Path& dirPath)
    {
        if (!m_fileSystem.isFile(filePath))
        {
            return;
        }

        std::string filename = Common::FileSystem::basename(filePath);
        m_fileSystem.copyFile(filePath, Common::FileSystem::join(dirPath, filename));
        LOGINFO("Copied " << filePath << " to " << dirPath);
    }

    void GatherFiles::copyFile(const Path& filePath, const Path& destination)
    {
        if (!m_fileSystem.isFile(filePath))
        {
            return;
        }

        m_fileSystem.copyFile(filePath, destination);
        LOGINFO("Copied " << filePath << " to " << destination);
    }

    void GatherFiles::copyAllOfInterestFromDir(const Path& dirPath, const Path& destination)
    {
        if (m_fileSystem.isDirectory(dirPath))
        {
            std::vector<std::string> files = m_fileSystem.listFiles(dirPath);
            for (const auto& file : files)
            {
                if (isFileOfInterest(file))
                {
                    copyFileIntoDirectory(file, destination);
                }
            }
        }
        else
        {
            LOGINFO("Directory does not exist or cannot be accessed: " << dirPath);
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
                copyFileIntoDirectory(filePath, destination);
            }
            else if (m_fileSystem.isDirectory(filePath))
            {
                copyAllOfInterestFromDir(filePath, destination);
            }
            else
            {
                LOGINFO("Skipping " << filePath << " is neither a file nor directory.");
            }
        }
    }

    std::vector<std::string> GatherFiles::getLogLocations(const Path& inputFilePath)
    {
        if (m_fileSystem.isFile(inputFilePath))
        {
            return m_fileSystem.readLines(inputFilePath);
        }
        throw std::invalid_argument("Error: Log locations config file does not exist: " + inputFilePath);
    }

    Path GatherFiles::getConfigLocation(const std::string& configFileName)
    {
        Path configFilePath = Common::FileSystem::join(m_installDirectory, "base/etc", configFileName);
        if (m_fileSystem.isFile(configFilePath))
        {
            LOGINFO("Location of config file: " << configFilePath);
            return configFilePath;
        }

        throw std::invalid_argument("Error: No config file - " + configFileName);
    }

    void GatherFiles::copyPluginSubDirectoryLogFiles(
        const Path& pluginsDir,
        const std::string& pluginName,
        const Path& destination)
    {
        static const std::vector<std::string> possiblePluginLogSubDirectories{ "dbos/data/logs" };

        // Copy all files from sub directories specified in possiblePluginLogSubDirectories
        for (const auto& possibleSubDirectory : possiblePluginLogSubDirectories)
        {
            std::string absolutePath = Common::FileSystem::join(pluginsDir, pluginName, possibleSubDirectory);

            LOGINFO(absolutePath.c_str());

            if (m_fileSystem.isDirectory(absolutePath))
            {
                std::string newDestinationPath =
                    Common::FileSystem::join(destination, pluginName, possibleSubDirectory);

                m_fileSystem.makedirs(newDestinationPath);

                std::vector<Path> files = m_fileSystem.listFiles(absolutePath);

                for (const auto& file : files)
                {
                    copyFileIntoDirectory(file, newDestinationPath);
                }
            }
        }
    }

    void GatherFiles::copyPluginFiles(const Path& destination)
    {
        // Find names of the plugins installed
        Path pluginsDir = Common::FileSystem::join(m_installDirectory, "plugins");

        if (!m_fileSystem.isDirectory(pluginsDir))
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

            copyPluginSubDirectoryLogFiles(pluginsDir, pluginName, destination);
        }
    }

    void GatherFiles::copyDiagnoseLogFile(const Path& destination)
    {
        // do not use copy file here because the logger will not be available.
        Path diagnoseLogPath = Common::FileSystem::join(m_installDirectory, "logs/base/diagnose.log");

        if (!m_fileSystem.isFile(diagnoseLogPath))
        {
            return;
        }

        Path fullDest = Common::FileSystem::join(destination, "BaseFiles/diagnose.log");

        if (!m_fileSystem.isFile(fullDest))
        {
            m_fileSystem.removeFile(fullDest);
        }

        m_fileSystem.copyFile(diagnoseLogPath, fullDest);
        std::cout << "Copied " << diagnoseLogPath << " to " << fullDest << std::endl;
    }

    void GatherFiles::copyFilesInComponentDirectories(const Path& destination)
    {

        std::vector<std::string> componentSubPaths =
                {"tmp/ServerProtectionLinux-Base", "tmp/ServerProtectionLinux-Plugin-MDR"};

        for(auto& componentSubPath : componentSubPaths)
        {
            Path sourcePath = Common::FileSystem::join(m_installDirectory, componentSubPath);

            if(!m_fileSystem.isDirectory(sourcePath))
            {
                continue;
            }

            std::vector<Path> files = m_fileSystem.listFiles(sourcePath);

            if(files.size() == 0)
            {
                continue;
            }

            std::string componentFolderName = Common::FileSystem::basename(sourcePath);

            Path destinationPath =  Common::FileSystem::join(destination, "BaseFiles", componentFolderName);

            if(!m_fileSystem.isDirectory(destinationPath))
            {
                m_fileSystem.makedirs(destinationPath);
            }

            for(auto& sourceFilePath : files)
            {
                Path destinationFilePath = Common::FileSystem::join(destinationPath, Common::FileSystem::basename(sourceFilePath));
                m_fileSystem.copyFile(sourceFilePath, destinationFilePath);
            }
        }
    }

} // namespace diagnose
