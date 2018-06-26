/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_DIRECTORYWATCHER_H
#define EVEREST_BASE_DIRECTORYWATCHER_H

#include <string>
#include <map>
#include <memory>
#include "Common/Threads/AbstractThread.h"
#include "IDirectoryWatcher.h"

namespace Common
{
namespace DirectoryWatcher
{
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
        explicit DirectoryWatcher(std::shared_ptr<IiNotifyWrapper> iNotifyWrapperPtr = std::make_shared<iNotifyWrapper>());
        ~DirectoryWatcher() override;
        void addListener(IDirectoryWatcherListener &watcherListener) override;
        void removeListener(IDirectoryWatcherListener &watcherListener) override;
        void startWatch() override;

    private:
        void run() override;

        bool m_watcherRunning;
        int m_inotifyFd;
        std::map<int, IDirectoryWatcherListener*> m_listenerMap;
        std::shared_ptr<IiNotifyWrapper> m_iNotifyWrapperPtr;
    };
}
}

#endif //EVEREST_BASE_DIRECTORYWATCHER_H


