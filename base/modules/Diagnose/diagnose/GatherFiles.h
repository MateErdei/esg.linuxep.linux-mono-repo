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

        void copyLogFiles(const Path& destination);
        void copyMcsConfigFiles(const Path& destination);
        std::string createDiagnoseFolder(const Path& path);
        void setInstallDirectory(const Path& path);

    private:

        Path getConfigLocation(const std::string& configFileName);
        std::vector<std::string> getLogLocations(const Path& inputFilePath);

        std::vector<std::string> m_logFilePaths;
        std::vector<std::string> m_mcsConfigDirectories;
        Common::FileSystem::FileSystemImpl m_fileSystem;
        std::string m_installDirectory;
    };

}
