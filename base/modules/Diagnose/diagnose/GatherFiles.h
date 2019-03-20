/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#pragma once

namespace diagnose
{
    class GatherFiles
    {
    public:
        GatherFiles() = default;

        /*
         * Copies all files of interest from the directories specified in "DiagnoseLogFilePaths.conf"
         * or any explicitly listed files to destination.
         */
        void copyBaseFiles(const Path& destination);

        /*
        * Copies all files of interest from the directories specified in "DiagnoseLogFilePaths.conf"
        * or any explicitly listed files to destination.
        */
        void copyPluginFiles(const Path& destination);

        /*
         * Creates the directory that the logs etc are copied to.
         */
        std::string createDiagnoseFolder(const Path& path);

        /*
         * Sets the install directory where diagnose will look for logs etc.
         */
        void setInstallDirectory(const Path& path);

    private:
        Path getConfigLocation(const std::string& configFileName);
        std::vector<std::string> getLogLocations(const Path& inputFilePath);
        void copyFile(const Path& filePath, const Path& destination);
        void copyAllOfInterestFromDir(const Path& filePath, const Path& destination);

        std::vector<std::string> m_logFilePaths;
        Common::FileSystem::FileSystemImpl m_fileSystem;
        std::string m_installDirectory;
    };
} // namespace diagnose
