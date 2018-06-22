/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_DIRECTORYWATCHER_H
#define EVEREST_BASE_DIRECTORYWATCHER_H

#include <string>
#include <map>
#include "Common/Threads/AbstractThread.h"
#include "IDirectoryWatcher.h"

struct DirectoryWatcherPair
{
    std::string directoryPath;
    std::function<void(std::string)> callback;
};

namespace Common
{
    namespace DirectoryWatcher
    {
        class DirectoryWatcher :  public virtual IDirectoryWatcher
        {
        public:
            DirectoryWatcher();
            ~DirectoryWatcher() override;
            void addWatch(std::string path, std::function<void(const std::string)> callbackFunction) override;

        private:
            void run() override;

            bool m_watcherRunning;
            int m_inotifyFd;
            std::map<int, DirectoryWatcherPair> m_callbackMap;
        };
    }
}

#endif //EVEREST_BASE_DIRECTORYWATCHER_H
