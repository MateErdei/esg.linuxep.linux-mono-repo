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
        virtual ~IDirectoryWatcherListener() = default;
        /**
         * @return Returns the absolute path used by Directory watcher to specify
         * the path that the listener is interested in.
         */
        virtual std::string getPath() const = 0;
        /**
         * Directory watcher informs the listener through this method
         * of the name of a file moved into the directory that the listener
         * is listening to. Receives just the filename.
         */
        virtual void fileMoved(const std::string & filename) = 0;
        /**
         * Directory watcher informs the listener through this method
         * if the directory is being actively watched.
         * True is sent on startWatch and on addition of a listener if
         * watcher is already started
         * False is sent on removal of listener from watcher
         */
        virtual void watcherActive(bool) = 0;
    };


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
        virtual void addListener(IDirectoryWatcherListener &) = 0;
        /**
         * Remove a listener from the directory watcher.
         * Call watcherActive(false) on listener.
         */
        virtual void removeListener(IDirectoryWatcherListener &) = 0;
        /**
         * Starts the main thread of the directory watcher.
         * This thread should actively watch the directories of the
         * listeners added. It should also return via fileMoved when
         * files are moved into the watched directories.
         * The thread also should call watchActive(true) on any listener.
         */
        virtual void startWatch() = 0;
    };
}
}

#endif //EVEREST_BASE_IDIRECTORYWATCHER_H