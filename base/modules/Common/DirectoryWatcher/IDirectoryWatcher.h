//
// Created by pair on 22/06/18.
//

#ifndef EVEREST_BASE_IDIRECTORYWATCHER_H
#define EVEREST_BASE_IDIRECTORYWATCHER_H

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
        class IDirectoryWatcher
        {
        public:
            virtual IDirectoryWatcher() = default;
        };
    }
}

#endif //EVEREST_BASE_IDIRECTORYWATCHER_H
