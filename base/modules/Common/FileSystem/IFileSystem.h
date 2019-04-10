/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

using Path = std::string;

namespace Common
{
    namespace FileSystem
    {
        /**
         * Joins 2 linux paths together
         * @param path1, first part the path
         * @param path2, second part of the path
         * @return string containing the joined path
         */
        Path join(const Path& path1, const Path& path2);

        Path join(const Path& path1, const Path& path2, const Path& path3);

        /**
         * Gets the last part i.e. file name from the given path
         * @param path to operate on.
         * @return the base name of the path, i.e. file name
         */
        std::string basename(const Path& path);

        /**
         * Gets the parent path for the given location.
         * @param path to operate on
         * @return the parent path based on the given path.
         */
        std::string dirName(const Path& path);

        class IFileSystem
        {
        public:
            virtual ~IFileSystem() = default;

            /**
             * Checks to see if the file or directory provided already exists
             * @param path to check
             * @return true, if file or directory exists, false otherwise
             */
            virtual bool exists(const Path& path) const = 0;

            /**
             * Checks the current process can execute the path
             * @param path to check
             * @return true, if file is executable otherwise false
             */
            virtual bool isExecutable(const Path& path) const = 0;

            /**
             * Checks to see if the given path is a regular file
             * @param path to check
             * @return true, if path is a regular file, false otherwise
             */
            virtual bool isFile(const Path& path) const = 0;

            /**
             * Checks to see if the given path is a directory.
             * @param path to check
             * @return true, if path directory exists, false otherwise
             */
            virtual bool isDirectory(const Path& path) const = 0;

            /**
             * Checks to see if the given path is a symlink.
             * @param path to check
             * @return true, if path is a symlink, false otherwise
             */
            virtual bool isSymlink(const Path& path) const = 0;

            /**
             * Gets the current working directory for the application.
             * @return path to the current working directory
             */
            virtual Path currentWorkingDirectory() const = 0;

            /**
             * Moves a file from one location to another
             * @param sourcePath, location of the file to be moved.
             * @param destPath, location of the file to move to.
             */
            virtual void moveFile(const Path& sourcePath, const Path& destPath) const = 0;

            /**
             * Reads a given file content into string.
             *
             * Limits the file read to 10 MiB
             *
             * @param path, location of the file to read.
             * @return the content of the read file.
             */
            virtual std::string readFile(const Path& path) const = 0;

            /**
             * Reads a given file content into string.
             * @param path, location of the file to read.
             * @param maxSize The maximum file size to read, throws exception if the file is larger than that
             * @return the content of the read file.
             */
            virtual std::string readFile(const Path& path, unsigned long maxSize) const = 0;

            /**
             * Reads the contents of a given file line by line and returns the strings in a vector.
             * @param path, location of the file to read.
             * @return the content of the read file split by new lines.
             */
            virtual std::vector<std::string> readLines(const Path& path) const = 0;

            /**
             * Writes the given string content into a new file.
             * @param path, location of the file to create
             * @param content, the string value to write into the given file.
             */
            virtual void writeFile(const Path& path, const std::string& content) const = 0;

            /**
             * Writes the given string content into a new file atomically.
             * @param path, location of the file to create
             * @param content, the string value to write into the given file.
             * @param tempDir, staging area where the file will be created before being moved to the correct place.
             */
            virtual void writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir)
                const = 0;

            /**
             * Provide the fullPath of the files under the directoryPath given.
             *
             * @note Only regular files are listed, directories or special file system entries will not be listed.
             *
             * @param directoryPath
             * @return List of the full path of files under the directoryPath.
             */
            virtual std::vector<Path> listFiles(const Path& directoryPath) const = 0;

            /**
             * Provide the fullPath of the files and directories under the directoryPath given.
             *
             * @note Only regular files and directories are listed, special file system entries will not be listed.
             * . and .. are not listed
             *
             * @param directoryPath
             * @param includeSymlinks If true then include symlinks
             * @return List of the full path of files under the directoryPath.
             */
            virtual std::vector<Path> listFilesAndDirectories(const Path& directoryPath, bool includeSymlinks=false) const = 0;

            /**
            * Provide the full absolute path of the directories under the directoryPath given.
            *
            * @note Only regular directories are listed, special file system entries will not be listed.
            * . and .. are not listed
            *
            * @param directoryPath
            * @return List of the full path of files under the directoryPath.
            */
            virtual std::vector<Path> listDirectories(const Path& directoryPath) const = 0;

            virtual void makeExecutable(const Path& path) const = 0;

            /**
             * Create a directory tree
             *
             * Ignore errors due to existing directories.
             * Recursively create parent directories as required
             *
             * @param path, the directory path that will be created
             */
            virtual void makedirs(const Path& path) const = 0;

            /**
             * Copy one file to another
             * @param src, source file that is to be copied
             * @param dest, location where the file will be copied to
             */
            virtual void copyFile(const Path& src, const Path& dest) const = 0;

            /**
             * Remove file from filesystem
             * @param path, full path to the file which is to be deleted
             */
            virtual void removeFile(const Path& path) const = 0;

            /**
             * Recursively delete a directory or file.
             * @param path, full path to the directory which is to be deleted
             */
            virtual void removeDirectory(const Path& path) const = 0;

            /**
             * Convert a relative path to an absolute path in the current working directory.
             *
             * /Foo -> /Foo
             * Foo/Bar -> <cwd>/Foo/Bar
             *
             * @param path
             * @return Absolute Path
             */
            virtual Path makeAbsolute(const Path& path) const = 0;

            /**
             * Follows a symbolic link and returns the file it is linked to
             * or an empty string if path doesn't point to a symbolic link
             *
             * @param pathToLink
             * @return pathLinkedTo
             */
            virtual Path readlink(const Path& path) const = 0;
        };

        /**
         * Return a BORROWED pointer to a static IFileSystem instance.
         *
         * Do not delete this yourself.
         *
         * @return BORROWED IFileSystem pointer
         */
        IFileSystem* fileSystem();
    } // namespace FileSystem
} // namespace Common
