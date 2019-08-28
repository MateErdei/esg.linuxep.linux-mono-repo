/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdint.h>
#include <unistd.h>
#include <memory>

namespace Common
{
    namespace DirectoryWatcher
    {
        class IiNotifyWrapper
        {
        public:
            virtual ~IiNotifyWrapper() = default;
            /**
             * Initialises and returns a file descriptor which receives iNotify like events
             */
            virtual int init() = 0;
            /**
             * Add an iNotify like watch to the specified file descriptor
             * @param fd - file descriptor
             * @param name - directory being watched
             * @param mask - contains bits that describe the event that is being watched
             * @return watch descriptor
             */
            virtual int addWatch(int fd, const char* name, uint32_t mask) = 0;
            /**
             * Removes a watch on a specific watch descriptor (as returned by add_watch)
             * @param fd - file descriptor
             * @param wd - watch descriptor
             * @return  0 on success
             */
            virtual int removeWatch(int fd, int wd) = 0;
            /**
             * Performs a read on the given file descriptor
             * @param fd - File descriptor
             * @param buf - Buffer being read into
             * @param nbytes - Number of bytes requested
             * @return  - Number of bytes read
             */
            virtual ssize_t read(int fd, void* buf, size_t nbytes) = 0;
        };

        using IiNotifyWrapperPtr = std::unique_ptr<IiNotifyWrapper>;
    } // namespace DirectoryWatcher
} // namespace Common
