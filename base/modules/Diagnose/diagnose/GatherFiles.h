/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>

namespace diagnose
{
    class GatherFiles
    {


    public:
        GatherFiles() = default;

        void copyLogFiles(std::string logLocationsFile,std::string destination);
        void copyMcsConfigFiles(std::string mcsFolderLocationFile, std::string destination);
        std::string createDiagnoseFolder(std::string path);
        std::vector<std::string> getLogLocations(std::string inputFilePath);
        void setInstallDirectory(std::string path);

    private:

        std::vector<std::string> m_logFilePaths;
        std::vector<std::string> m_mcsConfigDirectories;
        Common::FileSystem::FileSystemImpl m_fileSystem;
        std::string m_installDirectory;
    };

}
