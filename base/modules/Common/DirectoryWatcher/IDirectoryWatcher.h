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
         * @return Used by Directory watcher to specify the path that
         * the listener is interested in.
         */
        virtual std::string getPath() const = 0;
        /**
         * Directory watcher informs the listener through this method
         * of the name of a file moved into the directory that the listener
         * is listening to.
         */
        virtual void fileMoved(std::string const &) = 0;
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
         * @param __fd - file descriptor
         * @param __name - directory being watched
         * @param __mask - contains bits that describe the event that is being watched
         * @return watch descriptor
         */
        virtual int addWatch(int __fd, const char *__name, uint32_t __mask) = 0;
        /**
         * Removes a watch on a specific watch descriptor (as returned by add_watch)
         * @param __fd - file descriptor
         * @param __wd - watch descriptor
         * @return  removed watch descriptor
         */
        virtual int removeWatch(int __fd, int __wd) = 0;
        /**
         * Performs a read on the given file descriptor
         * @param __fd - File descriptor
         * @param __buf - Buffer being read into
         * @param __nbytes - Number of bytes requested
         * @return  - Number of bytes read
         */
        virtual ssize_t read(int __fd, void *__buf, size_t __nbytes) = 0;
    };
}
}

#endif //EVEREST_BASE_IDIRECTORYWATCHER_H