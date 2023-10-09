/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <memory>
#include <optional>
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
             * @return true, if path is a regular file (file, symlink), false otherwise
             */
            virtual bool isFile(const Path& path) const = 0;

            /**
             * Checks to see if the given path is a directory.
             * @param path to check
             * @return true, if path directory exists, false otherwise
             */
            virtual bool isDirectory(const Path& path) const = 0;

            /**
             * Checks to see if the given path is a file or directory.
             * @param path to check
             * @return true, if path is dir or a regular file, false otherwise
             */
            virtual bool isFileOrDirectory(const Path& path) const = 0;

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
             *
             * Limits the file read to 10 MiB
             *
             * @param path, location of the file to read.
             * @return the content of the read file split by new lines.
             */
            virtual std::vector<std::string> readLines(const Path& path) const = 0;

            /**
             * Reads the contents of a given file line by line and returns the strings in a vector.
             * @param path, location of the file to read.
             * @param maxSize The maximum file size to read, throws exception if the file is larger than that
             * @return the content of the read file split by new lines.
             */
            virtual std::vector<std::string> readLines(const Path& path, unsigned long maxSize) const = 0;

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
             * @param mode the chmod to be applied when the file is created (can be used to extend file to read group).
             * If set to 0, no permisison will be changed.
             */
            virtual void writeFileAtomically(
                const Path& path,
                const std::string& content,
                const Path& tempDir,
                mode_t mode) const = 0;
            /** Keep the interface without mode_t, by setting it to 0 (do not change it) */
            virtual void writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir)
                const = 0;

            /**
             * Appends the given string content to the given file. Function can throw.
             * @param path, location of the file to append to (the file will be created if it doesn't exist)
             * @param content, the string value to append into the given file, new lines are not added implicitly
             */
            virtual void appendFile(const Path& path, const std::string& content) const = 0;

            /**
             * Provide the fullPath of the files under the directoryPath given.
             *
             * @note Only regular files and symlinks are listed, directories or special file system entries will not be
             * listed.
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
            virtual std::vector<Path> listFilesAndDirectories(const Path& directoryPath, bool includeSymlinks = false)
                const = 0;
            /**
             * Provide the fullPath of the files under the directoryPath and in its sub directories
             *
             * @note Only regular files are listed, special file system entries will not be listed.
             * . and .. are not listed
             *
             * @param directoryPath
             * @return List of the full path of files under the directoryPath.
             */
            virtual std::vector<Path> listAllFilesInDirectoryTree(const Path& root) const = 0;

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
             * Copy one file to another, overwriting target if it exists.
             * @param src, source file that is to be copied
             * @param dest, location where the file will be copied to
             */
            virtual void copyFile(const Path& src, const Path& dest) const = 0;

            /**
             * Copy one file to another, and set file permissions and owners
             * @param src, source file that is to be copied
             * @param dest, location where the file will be copied to
             * @param mode, Permissions of file
             * @param ownerName, the name of the user which will be set as file owner
             * @param groupName, the name of the group which will be set as file group
             */
            virtual void copyFileAndSetPermissions(
                const Path& src,
                const Path& dest,
                mode_t mode,
                const std::string& ownerName,
                const std::string& groupName) const = 0;

            /**
             * Remove file from filesystem
             * @param path, full path to the file which is to be deleted
             * @param ignoreAbsent  ignore ENOENT error from unlink
             */
            virtual void removeFile(const Path& path, bool ignoreAbsent) const = 0;

            /**
             * Remove file from filesystem
             * @param path, full path to the file which is to be deleted
             */
            virtual void removeFile(const Path& path) const = 0;

            /**
             * Recursively delete a directory or file.
             * @param path, full path to the directory which is to be deleted
             */
            virtual void removeFileOrDirectory(const Path& path) const = 0;

            /**
             * Delete all files in the given path
             * @param path of the files to delete
             */
            virtual void removeFilesInDirectory(const Path& path) const = 0;

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
            virtual std::optional<Path> readlink(const Path& path) const = 0;

            /**
             * Return the size in bytes of the file at path location
             * @param path
             * @return
             */
            virtual off_t fileSize(const Path& path) const = 0;

            /**
             * Return the time the file was last modified
             * @param path
             * @return
             */
            virtual std::time_t lastModifiedTime(const Path& path) const = 0;
            /**
             * Wait for the given file to exist.
             * @param path, location of the file to wait for
             * @param timeout, maximum time in milliseconds to wait for the file to exist before returning
             *@return true if file exists within given time, false if the file did not exist within the given time
             */
            virtual bool waitForFile(const Path& path, unsigned int timeout) const = 0;

            /**
             * Removes the contents of a directory without removing the directory itself
             * @param path to the directory to remove the contents of
             */
            virtual void recursivelyDeleteContentsOfDirectory(const Path& path) const = 0;

            /** Return the content of
             * /proc/<pid>/<filename>
             * Given the nature of proc files being ephemeral, it returns optional which means that if the value is not
             * available or it fail to read the content, it will return an empty optional value.
             *
             * Look that fileSystem()->readContent can not be used as it tries to hold the size of the file which is not
             * valid for /proc files.
             * @param pid
             * @param filename
             * @return
             */
            virtual std::optional<std::string> readProcFile(int pid, const std::string& filename) const = 0;
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
