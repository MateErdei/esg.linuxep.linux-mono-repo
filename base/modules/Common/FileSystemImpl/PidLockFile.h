/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/FileSystem/IPidLockFileUtils.h"

#include <memory>
#include <string>

namespace Common
{
    namespace FileSystemImpl
    {
        class PidLockFileUtils : public Common::FileSystem::IPidLockFileUtils
        {
        public:
            int open(const std::string& pathname, int flags, mode_t mode) const override;
            int flock(int fd) const override;
            int ftruncate(int fd, off_t length) const override;
            ssize_t write(int fd, const void* buf, size_t count) const override;
            void close(int fd) const override;
            void unlink(const std::string& pathname) const override;
            __pid_t getpid() const override;
        };

        class PidLockFile : public Common::FileSystem::ILockFileHolder
        {
            PidLockFile& operator=(const PidLockFile&) = delete;
            PidLockFile(const PidLockFile&) = delete;

        public:
            /**
             * Attempts to creates a file lock at pidfile and write the pid to it and throws
             * if it fails.
             * @param pidfile
             */
            explicit PidLockFile(const std::string& pidfile);
            /**
             * Closes the pidfile and deletes it.
             */
            virtual ~PidLockFile();

            [[nodiscard]] std::string filePath() const override;

        private:
            int m_fileDescriptor;
            std::string m_pidfile;
        };

        // Used for testing only
        using IPidLockFileUtilsPtr = std::unique_ptr<Common::FileSystem::IPidLockFileUtils>;
        void replacePidLockUtils(IPidLockFileUtilsPtr pointerToReplace);
        void restorePidLockUtils();
    } // namespace FileSystemImpl
} // namespace Common
