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
        class IDirectoryWatcher : public Common::Threads::AbstractThread
        {
        public:
            virtual ~IDirectoryWatcher() = default;
            virtual void addWatch(std::string path, std::function<void(const std::string)> callbackFunction) = 0;
        };
    }
}

#endif //EVEREST_BASE_IDIRECTORYWATCHER_H