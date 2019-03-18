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
        const size_t lastSlashPos = string.find_last_of("/");
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
    void GatherFiles::setInstallDirectory(std::string path)
    {
        m_installDirectory = path;
    }
    std::string GatherFiles::createDiagnoseFolder(std::string path )
    {

        if(m_fileSystem.isDirectory(path))
        {
            std::cout << "Directory path already exists: " << path << std::endl;
        }
        m_fileSystem.makedirs(path);

        if(m_fileSystem.isDirectory(path))
        {
            return path;
        }
        throw std::invalid_argument("Directory was not created");
    }

    void GatherFiles::copyLogFiles(std::string logLocationsFile, std::string destination)
    {
        m_logFilePaths = getLogLocations(logLocationsFile);
        for(const auto path: m_logFilePaths)
        {
            std::string filePath = m_installDirectory+'/'+path;
            if(m_fileSystem.isFile(filePath))
            {
                std::cout <<  filePath << std::endl;
                std::cout <<  destination << std::endl;
                std::string filename = getFileName(filePath);
                m_fileSystem.copyFile(filePath,destination+filename);
            }
            else
            {
                std::cout << "file not found " << filePath << std::endl;
            }
        }
    }

    void GatherFiles::copyMcsConfigFiles(std::string mcsFolderLocationFile, std::string destination)
    {
        m_mcsConfigDirectories = getLogLocations(mcsFolderLocationFile);
        for(const auto path: m_mcsConfigDirectories)
        {
            std::string dirPath = m_installDirectory+path;
            if(m_fileSystem.isDirectory(dirPath))
            {
                std::vector<std::string> files = m_fileSystem.listFiles(dirPath);

                for(const auto file : files)
                {

                    if(stringEndsWith(file,".XML"))
                    {
                        std::string filename = getFileName(file);
                        m_fileSystem.copyFile(file,destination+filename);
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

    std::vector<std::string> GatherFiles::getLogLocations(std::string inputFilePath)
    {
        std::vector<std::string> listOfLogPaths;

        Common::FileSystem::FileSystemImpl fileSystem;
        listOfLogPaths  = fileSystem.readLines(inputFilePath);
        return listOfLogPaths;

    }
}
