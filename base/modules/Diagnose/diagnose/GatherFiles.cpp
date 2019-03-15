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
}

namespace diagnose
{
    std::string GatherFiles::createDiagnoseFolder()
    {
        //const char dir_path[] = "/temp1";

        std::string path = "/tmp/temp";

        if(m_fileSystem.isDirectory(path))
        {
            throw std::invalid_argument("Directory path already exists: " + path);
        }
        m_fileSystem.makedirs(path);

        if(m_fileSystem.isDirectory(path))
        {
            return path;
        }
        throw std::invalid_argument("Directory was not created");
    }

    void GatherFiles::copyLogFiles(std::string InstallLocation)
    {
        std::string destination = createDiagnoseFolder();

        for(const auto path: m_logFilePaths)
        {
            std::string filePath = InstallLocation+'/'+path;
            if(m_fileSystem.isFile(filePath))
            {
                m_fileSystem.copyFile(filePath,destination);
            }
            else
            {
                std::cout << "file not found " << filePath << std::endl;
            }
        }
    }

    void GatherFiles::copyMcsConfigFiles(std::string InstallLocation)
    {
        std::string destination = createDiagnoseFolder();

        for(const auto path: m_mcsConfigDirectories)
        {
            std::string dirPath = InstallLocation+path;
            if(m_fileSystem.isDirectory(dirPath))
            {
                std::vector<std::string> files = m_fileSystem.listFiles(dirPath);
                for(const auto file : files)
                {
                    if(stringEndsWith(file,".XML"))
                    {
                        m_fileSystem.copyFile(file,destination);
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
}
