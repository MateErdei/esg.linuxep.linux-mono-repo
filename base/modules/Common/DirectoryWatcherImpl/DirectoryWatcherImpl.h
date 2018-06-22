/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_DIRECTORYWATCHER_H
#define EVEREST_BASE_DIRECTORYWATCHER_H

#include <string>
#include <deque>
#include <map>
#include "Common/Threads/AbstractThread.h"

struct DirectoryWatcherPair
{
    std::string directoryPath;
    std::function<void(std::string)> callback;
};

namespace Common
{
    namespace DirectoryWatcher
    {
        class DirectoryWatcher : public Common::Threads::AbstractThread
        {
        public:
            DirectoryWatcher();
            ~DirectoryWatcher() override;
            void addWatch(std::string path, std::function<void(const std::string)> callbackFunction);
        private:
            void run() override;

            bool m_watcherRunning;
            int m_inotifyFd;
            std::map<int, DirectoryWatcherPair> m_callbackMap;
        };
    }
}

#endif //EVEREST_BASE_DIRECTORYWATCHER_H
