/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IDIRECTORYWATCHER_H
#define EVEREST_BASE_IDIRECTORYWATCHER_H

#include <string>
#include "Common/Threads/AbstractThread.h"


namespace Common
{
    namespace DirectoryWatcher
    {
        class IDirectoryWatcherListener
        {
        public:
            virtual std::string getPath() const = 0;
            virtual void fileAdded(std::string const &) = 0;
            virtual void watcherClosed() = 0;
        };


        class IDirectoryWatcher
        {
        public:
            virtual void addListener(IDirectoryWatcherListener &) = 0;
            virtual void removeListener(IDirectoryWatcherListener &) = 0;
            virtual void startWatch() = 0;
            virtual void endWatch() = 0;
        };

        class IiNotifyWrapper
        {
        public:
            virtual int init() = 0;
            virtual int add_watch(int __fd, const char *__name, uint32_t __mask) = 0;
        };
    }
}

#endif //EVEREST_BASE_IDIRECTORYWATCHER_H