/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_FILESYSTEM_H
#define EVEREST_BASE_FILESYSTEM_H

#include "Common/FileSystem/IFileSystem.h"

namespace Common
{
    namespace FileSystem
    {
        class FileSystemImpl : public IFileSystem
        {
        public:
            FileSystemImpl() = default;
            ~FileSystemImpl() = default;

            Path join(const Path& path1, const Path & path2)const override;

            std::string basename(const Path & path ) const override;

            std::string dirName(const Path & path) const override;

            void copyFile(const Path& src, const Path& dest) const override;

            void copyPermissions( const Path& src, const Path & dest) const override;

            bool exists(const Path &path) const override;

            bool isExecutable(const Path &path) const override;

            bool isFile(const Path & path) const override;

            bool isDirectory(const Path & path) const  override;

            Path currentWorkingDirectory() const override;

            void moveFile(const Path &sourcePath, const Path &destPath) const override;

            std::string readFile(const Path &path) const override;

            void writeFile(const Path &path, const std::string &content) const override;

            void writeFileAtomically(const Path &path, const std::string &content, const Path &tempDir) const override;

            void makeExecutable(const Path &path) const override;

            void makedirs(const Path &path) const override;

            Path join(const Path &path1, const Path &path2, const Path &path3) const override;

            std::vector<Path> listFiles( const Path & directoryPath ) const override;
        };

        /** To be used in tests only */
        using IFileSystemPtr = std::unique_ptr<Common::FileSystem::IFileSystem>;
        void replaceFileSystem(IFileSystemPtr);
        void restoreFileSystem();
    }
}


#endif //EVEREST_BASE_FILESYSTEM_H
