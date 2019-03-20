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
         * Creates directories
         */
        std::string createDiagnoseFolder(const Path& path, std::string dirName);

        /*
         * Creates and returns the main diagnose output folder.
         */
        Path createRootDir(const Path& path);

        /*
         * Creates and returns the output folder for base files.
         */
        Path createBaseFilesDir(const Path& path);

        /*
         * Creates and returns the output folder for plugin files.
         */
        Path createPluginFilesDir(const Path& path);

        /*
         * Creates and returns the output folder for system files and output of system commands.
         */
        Path createSystemFilesDir(const Path& path);

        /*
         * Sets the install directory where diagnose will look for logs etc.
         */
        void setInstallDirectory(const Path& path);

        /*
         * Copies files with endings ".xml", ".json", ".txt", ".conf", ".config",
         * ".log", ".log.1", ".log.2", ".log.3", ".log.4", ".log.5", ".log.6", ".log.7", ".log.8", ".log.9"
         * from directory dirPath to the destination folder
         */
        void copyAllOfInterestFromDir(const Path& dirPath, const Path& destination);

        /*
         * Copies an existing file to the destination file.
         */
        void copyFile(const Path& filePath, const Path& destination);

    private:
        Path getConfigLocation(const std::string& configFileName);
        std::vector<std::string> getLogLocations(const Path& inputFilePath);

        std::vector<std::string> m_logFilePaths;
        Common::FileSystem::FileSystemImpl m_fileSystem;
        std::string m_installDirectory;
    };
} // namespace diagnose
