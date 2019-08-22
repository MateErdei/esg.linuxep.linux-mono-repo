/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>

namespace Common
{
    namespace OSUtilities
    {
        /** ILockFileHolder is an scoped class that on construction will acquire a lock file and
         * release on its destruction. It is mainly thought for applications that need to secure
         * a 'singleton app'. */
        class ILockFileHolder
        {
        public:
            virtual ~ILockFileHolder() = default;
        };

        class IPidLockFileUtils
        {
        public:
            virtual ~IPidLockFileUtils() = default;
            /**
             * Wrapper for open
             * @param pathname
             * @param flags
             * @param mode
             * @return
             */
            virtual int open(const std::string& pathname, int flags, mode_t mode) const = 0;
            /**
             * Wrapper for lockf
             * @param fd
             * @param cmd
             * @param len
             * @return
             */
            virtual int lockf(int fd, int cmd, off_t len) const = 0;
            /**
             * Wrapper for ftruncate
             * @param fd
             * @param length
             * @return
             */
            virtual int ftruncate(int fd, off_t length) const = 0;
            /**
             * Wrapper for write
             * @param fd
             * @param buf
             * @param count
             * @return
             */
            virtual ssize_t write(int fd, const void* buf, size_t count) const = 0;
            /**
             * Wrapper for close
             * @param fd
             */
            virtual void close(int fd) const = 0;
            /**
             * Wrapper for unlink
             * @param pathname
             */
            virtual void unlink(const std::string& pathname) const = 0;
            /**
             * Wrapper for getpid
             * @return
             */
            virtual __pid_t getpid() const = 0;
        };

        // Used to access global instance of PidLockUtils.
        IPidLockFileUtils* pidLockUtils();
        std::unique_ptr<ILockFileHolder> acquireLockFile(const std::string& fullPath);

    } // namespace OSUtilities
} // namespace Common
