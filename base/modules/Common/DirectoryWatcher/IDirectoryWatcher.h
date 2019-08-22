/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IDirectoryWatcherListener.h"

#include <memory>
#include <string>

namespace Common
{
    namespace DirectoryWatcher
    {
        class IDirectoryWatcher
        {
        public:
            virtual ~IDirectoryWatcher() = default;
            /**
             * Add a listener to the directory watcher.
             * Call watcherActive(true) on listener if the watch is active.
             * The caller is responsible for ensuring a listener is:
             *  - removed from the watcher before it is deleted.
             *  - still in memory when the directory watcher is destroyed.
             */
            virtual void addListener(IDirectoryWatcherListener&) = 0;
            /**
             * Remove a listener from the directory watcher.
             * Call watcherActive(false) on listener.
             */
            virtual void removeListener(IDirectoryWatcherListener&) = 0;
            /**
             * Starts the main thread of the directory watcher.
             * This thread should actively watch the directories of the
             * listeners added. It should also return via fileMoved when
             * files are moved into the watched directories.
             * The thread also should call watchActive(true) on any listener.
             */
            virtual void startWatch() = 0;
        };

        using IDirectoryWatcherPtr = std::unique_ptr<IDirectoryWatcher>;
        extern IDirectoryWatcherPtr createDirectoryWatcher();
    } // namespace DirectoryWatcher
} // namespace Common
