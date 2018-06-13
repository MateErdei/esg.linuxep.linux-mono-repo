/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IFILESYSTEM_H
#define EVEREST_BASE_IFILESYSTEM_H

#include <string>

using Path = std::string;

namespace Common
{
    namespace FileSystem
    {
        class IFileSystem
        {
        public:
            virtual Path join(const Path& path1, const Path & path2)const = 0;

            virtual std::string basename(const Path & path ) const = 0;

            virtual bool exists(const Path &path) const = 0;

            virtual bool isDirectory(const Path & path) const  = 0;

            virtual Path currentWorkingDirectory() const = 0;

            virtual void moveFile(const Path &sourcePath, const Path &destPath) = 0;

            virtual std::string readFile(const Path &path) const =0;

            virtual void writeFile(const Path &path, const std::string &content) =0;

            virtual void writeFileAtomically(const Path &path, const std::string &content, const Path &tempDir)=0;
        };
    }
}


#endif //EVEREST_BASE_IFILESYSTEM_H
