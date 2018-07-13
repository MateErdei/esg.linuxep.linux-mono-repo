///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_TESTS_TEMPDIR_H
#define EVEREST_BASE_TESTS_TEMPDIR_H

#include <string>
#include <vector>
#include <memory>
#include "Common/FileSystem/IFileSystem.h"

namespace Tests
{
    /**
     * Creates a temporary directory and provide some helper functions to setup files, directory structures and to
     * retrieve their path.
     */
    class TempDir
    {
    public:

        /**
         * Helper function. Equivalent to
         * std::unique_ptr<TempDir> tempDirPtr = std::unique_ptr<TempDir>(new TempDir());
         * @return Unique pointer to the temporary directory object.
         */
        static std::unique_ptr<TempDir> makeTempDir(const std::string & nameprefix = "temp");


        /**
         * Create a directory with permission 0700 on top of the baseDirectory with name starting with the namePrefix + 6 random characters.
         * The directory will be cleared on destruction.
         *
         * @param baseDirectory the directory to start with (it must exist). If empty, the current directory will be used.
         * @param namePrefix any string (not containing /) can be used and it will be used as the prefix of the directory name.
         */
        explicit  TempDir( const std::string & baseDirectory = "", const std::string & namePrefix = "temp");
        ~TempDir();
        /**
         * It handles resources, should not be copied.
         */
        TempDir& operator=(const TempDir & ) = delete;
        TempDir(const TempDir& ) = delete;


        /**
         *
         * @return The Absolute path of the temporary directory
         */
        std::string dirPath() const;

        /**
         * Get the full path from a relative path
         * @param relativePath
         * @return
         */
        std::string absPath( const std::string& relativePath) const;

        /**
         * It will make dirs, recursively if necessary.
         *
         * Example of calling makeDirs:
         * @code
         *
         * makeDirs( "level1/level2/level3") ;
         * @endcode
         *
         * Even if level1, does not exists at the moment of the call. All the necessary directories will be created.
         *
         * @param relativePath
         */
        void makeDirs( const std::string& relativePath) const;

        /**
         * Helper function that is equivalent to:
         * for ( auto & relativePath: relativePaths)
         *   makeDirs(relativePath);
         *
         * @param relativePaths
         */
        void makeDirs( const std::vector<std::string>& relativePaths) const;


        /**
         * It creates the file pointing to the relative path with its content matching the content string.
         * If the intermediate directories do not exist, it will be created during this call.
         * @param relativePath
         * @param content
         */
        void createFile( const std::string& relativePath, const std::string& content) const;

        /**
         * Helper class. Equivalent to:
         *
         * @code
         * std::string absPath = tempDir.absPath(relativePath);
         * auto fileSystem = createFileSystem();
         * return fileSystem->readFile(absPath);
         * @endcode
         *
         * @param relativePath path to the file to read relative to the temporary directory.
         * @return Content of the file pointed to relativePath
         * @throws IFileSystemException if the file can not be read.
         */
        std::string fileContent( const std::string & relativePath) const;

        /**
         * Auxiliary function to change the chmod of a file to enable execution.
         * It will make the file chmod 0700.
         * @param relativePath
         */
        void makeExecutable( const std::string & relativePath) const;

        /**
         * Helper method, equivalent to:
         *
         * std::string absPath = tempDir.absPath(relativePath);
         * auto fileSystem = createFileSystem();
         * return fileSystem->exists(absPath);
         *
         * @param relativePath
         */
        bool exists(const std::string & relativePath) const;

    private:
        static std::vector<std::string> pathParts( const std::string & relativePath);
        std::unique_ptr<Common::FileSystem::IFileSystem> m_fileSystem;
        std::string m_rootpath;


    };

}



#endif //EVEREST_BASE_TESTS_TEMPDIR_H
