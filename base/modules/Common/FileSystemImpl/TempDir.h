/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/ITempDir.h>

#include <string>

namespace Common
{
    namespace FileSystemImpl
    {
        class TempDir : public virtual Common::FileSystem::ITempDir
        {
        public:
            TempDir(const std::string& baseDir, const std::string& prefix);
            ~TempDir() override;
            Path dirPath() const override { return m_tempdir; }

        private:
            void deleteTempDir();

            Path m_tempdir;
        };
    } // namespace FileSystemImpl
} // namespace Common
