/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/IPidLockFileUtils.h>

#include <memory>
#include <string>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        class PidLockFileUtils : public Common::OSUtilities::IPidLockFileUtils
        {
        public:
            int open(const std::string& pathname, int flags, mode_t mode) const override;
            int lockf(int fd, int cmd, off_t len) const override;
            int ftruncate(int fd, off_t length) const override;
            ssize_t write(int fd, const void* buf, size_t count) const override;
            void close(int fd) const override;
            void unlink(const std::string& pathname) const override;
            __pid_t getpid() const override;
        };

        class PidLockFile : public Common::OSUtilities::ILockFileHolder
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

        private:
            int m_fileDescriptor;
            std::string m_pidfile;
        };

        // Used for testing only
        using IPidLockFileUtilsPtr = std::unique_ptr<Common::OSUtilities::IPidLockFileUtils>;
        void replacePidLockUtils(IPidLockFileUtilsPtr pointerToReplace);
        void restorePidLockUtils();
    } // namespace OSUtilitiesImpl
} // namespace Common
