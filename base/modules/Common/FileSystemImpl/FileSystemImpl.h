/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_FILESYSTEM_H
#define EVEREST_BASE_FILESYSTEM_H

#include "IFileSystem.h"

namespace Common
{
    namespace FileSystem
    {
        class FileSystemImpl : public IFileSystem
        {
        public:
            FileSystemImpl();
            ~FileSystemImpl();

            Path join(const Path& path1, const Path & path2)const override;

            std::string basename(const Path & path ) const override;

            std::string dirName(const Path & path) const override;

            bool exists(const Path &path) const override;

            bool isDirectory(const Path & path) const  override;

            Path currentWorkingDirectory() const override;

            void moveFile(const Path &sourcePath, const Path &destPath) const override;

            std::string readFile(const Path &path) const override;

            void writeFile(const Path &path, const std::string &content) const override;

            void writeFileAtomically(const Path &path, const std::string &content, const Path &tempDir) const override;
        };
    }
}


#endif //EVEREST_BASE_FILESYSTEM_H
