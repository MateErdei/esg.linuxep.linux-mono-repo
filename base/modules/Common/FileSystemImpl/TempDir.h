/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/FileSystem/IFileSystem.h>

#include <string>

namespace Common
{
    namespace FileSystemImpl
    {
        class TempDir
        {
        public:
            TempDir(const std::string& baseDir, const std::string& prefix);
            ~TempDir();
            Path dirPath() const
            {
                return m_tempdir;
            }

        private:
            void deleteTempDir();
            void deleteDirectory(const std::string& dir, Common::FileSystem::IFileSystem* filesystem=nullptr);

            Path m_tempdir;
        };
    }
}



