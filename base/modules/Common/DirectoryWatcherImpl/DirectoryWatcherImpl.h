/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_DIRECTORYWATCHER_H
#define EVEREST_BASE_DIRECTORYWATCHER_H

#include <string>
#include <map>
#include <memory>
#include "Common/Threads/AbstractThread.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"

using namespace Common::DirectoryWatcher;

namespace Common
{
namespace DirectoryWatcherImpl
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
        virtual int addWatch(int fd, const char *name, uint32_t mask) = 0;
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
        virtual ssize_t read(int fd, void *buf, size_t nbytes) = 0;
    };

    class iNotifyWrapper : public virtual IiNotifyWrapper
    {
    public:
        iNotifyWrapper() = default;
        int init() override;
        int addWatch(int fd, const char *name, uint32_t mask) override;
        int removeWatch(int fd, int wd) override;
        ssize_t read(int fd, void *buf, size_t nbytes) override;
    };

class DirectoryWatcher :  public virtual IDirectoryWatcher, public Common::Threads::AbstractThread
    {
    public:
        explicit DirectoryWatcher(std::unique_ptr<IiNotifyWrapper> iNotifyWrapperPtr = std::unique_ptr<IiNotifyWrapper>(new iNotifyWrapper()));
        ~DirectoryWatcher() override;
        void addListener(IDirectoryWatcherListener &watcherListener) override;
        void removeListener(IDirectoryWatcherListener &watcherListener) override;
        void startWatch() override;

    private:
        void run() override;

        bool m_watcherRunning;
        int m_inotifyFd;
        std::map<int, IDirectoryWatcherListener*> m_listenerMap;
        std::unique_ptr<IiNotifyWrapper> m_iNotifyWrapperPtr;
    };
}
}

#endif //EVEREST_BASE_DIRECTORYWATCHER_H


