/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include "Common/DirectoryWatcher/IiNotifyWrapper.h"
#include "Common/Threads/AbstractThread.h"

#include <map>
#include <memory>
#include <string>

using namespace Common::DirectoryWatcher;

namespace Common
{
    namespace DirectoryWatcherImpl
    {
        class iNotifyWrapper : public virtual IiNotifyWrapper
        {
        public:
            iNotifyWrapper() = default;
            int init() override;
            int addWatch(int fd, const char* name, uint32_t mask) override;
            int removeWatch(int fd, int wd) override;
            ssize_t read(int fd, void* buf, size_t nbytes) override;
        };

        class DirectoryWatcher : public virtual IDirectoryWatcher, public Common::Threads::AbstractThread
        {
        public:
            explicit DirectoryWatcher(
                std::unique_ptr<IiNotifyWrapper> iNotifyWrapperPtr =
                    std::unique_ptr<IiNotifyWrapper>(new iNotifyWrapper()));
            ~DirectoryWatcher() override;
            void addListener(IDirectoryWatcherListener& watcherListener) override;
            void removeListener(IDirectoryWatcherListener& watcherListener) override;
            void startWatch() override;

        private:
            void run() override;

            bool m_watcherRunning;
            int m_inotifyFd;
            std::map<int, IDirectoryWatcherListener*> m_listenerMap;
            std::unique_ptr<IiNotifyWrapper> m_iNotifyWrapperPtr;
        };
    } // namespace DirectoryWatcherImpl
} // namespace Common
