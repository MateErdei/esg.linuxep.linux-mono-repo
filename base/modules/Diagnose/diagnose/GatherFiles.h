// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/ITempDir.h"

#pragma once

namespace diagnose
{
    class GatherFiles
    {
    public:
        explicit GatherFiles(std::unique_ptr<Common::FileSystem::IFileSystem> filesystem);

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
         * ".log", ".log.1", ".dat", ".yaml"
         * from directory dirPath to the destination folder
         */
        void copyAllOfInterestFromDir(const Path& dirPath, const Path& destination);

        /*
         * Copies an existing file to the destination file.
         */
        void copyFile(const Path& filePath, const Path& destination);

        /*
         * Copies an existing file to the destination directory.
         */
        void copyFileIntoDirectory(const Path& filePath, const Path& dirPath);

        /*
         * Copies component specific directories to destination directory
         */
        void copyFilesInComponentDirectories(const Path& destination);

        /*
         * Copies the diagnose log file to the destination directory.
         */
        void copyDiagnoseLogFile(const Path& destination);

        /*
         * Returns the location the diagnose files are being gathered in
         */
        Path getRootLocation();

    protected:
        /*
         * Creates directories
         */
        std::string createDiagnoseFolder(const Path& path, const std::string& dirName);

        Path getConfigLocation(const std::string& configFileName);
        std::vector<std::string> getLogLocations(const Path& inputFilePath);

        /*
         * Copies all plugin files (expected to be log files) from a predefined set of sub-directories to the
         * destination directory. If the predefined sub-directory does not exist, it is ignored.  The sub-directory
         * structure is maintained when copying to the destination folder.
         */
        void copyPluginSubDirectoryFiles(
            const Path& pluginsDir,
            const std::string& pluginName,
            const Path& destination);

        std::vector<std::string> m_logFilePaths;
        std::unique_ptr<Common::FileSystem::IFileSystem> m_fileSystem;
        std::string m_installDirectory;
        std::unique_ptr<Common::FileSystem::ITempDir> m_tempDir;
    };
} // namespace diagnose
