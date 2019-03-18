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

        void copyLogFiles(Path destination);
        void copyMcsConfigFiles(Path destination);
        std::string createDiagnoseFolder(Path path);
        std::vector<std::string> getLogLocations(Path inputFilePath);
        void setInstallDirectory(Path path);
        Path getConfigLocation(std::string configFileName);

    private:

        std::vector<std::string> m_logFilePaths;
        std::vector<std::string> m_mcsConfigDirectories;
        Common::FileSystem::FileSystemImpl m_fileSystem;
        std::string m_installDirectory;
    };

}
