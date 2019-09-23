/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DirectoryWatcherImpl.h"

#include "Logger.h"

#include <Common/DirectoryWatcher/IDirectoryWatcherException.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include <sys/inotify.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <thread>
#include <unistd.h>

namespace Common
{
    namespace DirectoryWatcherImpl
    {
        int iNotifyWrapper::init() { return ::inotify_init(); }

        int iNotifyWrapper::addWatch(int fd, const char* name, uint32_t mask)
        {
            return ::inotify_add_watch(fd, name, mask);
        }

        int iNotifyWrapper::removeWatch(int fd, int wd) { return ::inotify_rm_watch(fd, wd); }

        ssize_t iNotifyWrapper::read(int fd, void* buf, size_t nbytes) { return ::read(fd, buf, nbytes); }

        DirectoryWatcher::DirectoryWatcher(std::unique_ptr<IiNotifyWrapper> iNotifyWrapperPtr) :
            m_iNotifyWrapperPtr(std::move(iNotifyWrapperPtr))
        {
            m_watcherRunning = false;
            m_inotifyFd = m_iNotifyWrapperPtr->init();
            if (m_inotifyFd == -1)
            {
                throw IDirectoryWatcherException("iNotify init failed.");
            }
        }

        DirectoryWatcher::~DirectoryWatcher()
        {
            requestStop();
            join();
            close(m_inotifyFd);
        }

        void DirectoryWatcher::addListener(IDirectoryWatcherListener& watcherListener)
        {
            std::string path = watcherListener.getPath();
            int watch = m_iNotifyWrapperPtr->addWatch(
                m_inotifyFd, path.c_str(), IN_MOVED_TO); // Only interested in files moved to the folder
            if (watch == -1)
            {
                throw IDirectoryWatcherException(
                    "Failed to add a Listener to Directory Watcher. Path: " + watcherListener.getPath());
            }
            m_listenerMap.emplace(watch, &watcherListener);
            watcherListener.watcherActive(m_watcherRunning);
        }

        void DirectoryWatcher::removeListener(IDirectoryWatcherListener& watcherListener)
        {
            auto delIter = std::find_if(
                m_listenerMap.begin(),
                m_listenerMap.end(),
                [&watcherListener](const std::pair<int, const IDirectoryWatcherListener*>& pair) {
                    return pair.second == &watcherListener;
                });
            if (delIter != m_listenerMap.end())
            {
                int watchDescRemoved = m_iNotifyWrapperPtr->removeWatch(m_inotifyFd, delIter->first);
                assert(watchDescRemoved == 0);
                static_cast<void>(watchDescRemoved);
                m_listenerMap.erase(delIter);
            }
            else
            {
                throw IDirectoryWatcherException(
                    "Failed to remove a Listener from the Directory Watcher. Path: " + watcherListener.getPath());
            }
            watcherListener.watcherActive(false);
        }

        void DirectoryWatcher::startWatch()
        {
            if (!m_watcherRunning)
            {
                start();
            }
        }

        void DirectoryWatcher::run()
        {
            m_watcherRunning = true;
            for (auto& iter : m_listenerMap)
            {
                iter.second->watcherActive(m_watcherRunning);
            }
            announceThreadStarted();
            // Setup poller with the inotify file descriptor and a notify pipe so we are able to quit the thread.
            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
            Common::ZeroMQWrapper::IHasFDPtr iNotifyFD =
                poller->addEntry(m_inotifyFd, Common::ZeroMQWrapper::IPoller::POLLIN);
            Common::ZeroMQWrapper::IHasFDPtr notifyPipe =
                poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            bool exit = false;
            while (!exit)
            {
                char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
                /* Some systems cannot read integer variables if they are not
                properly aligned. On other systems, incorrect alignment may
                decrease performance. Hence, the buffer used for reading from
                the inotify file descriptor should have the same alignment as
                struct inotify_event. */
                ;
                const struct inotify_event* event;
                Common::ZeroMQWrapper::IPoller::poll_result_t entries;
                try
                {
                    entries = poller->poll(std::chrono::milliseconds(-1));
                }
                catch (Common::ZeroMQWrapperImpl::ZeroMQPollerException& ex)
                {
                    LOGWARN(ex.what());
                    entries.clear();
                    exit = true;
                }

                // Only two file descriptors being polled and the notify pipe is only used for stopping thread

                if (stopRequested())
                {
                    // notifyPipe has been written. Exit the thread.
                    exit = true;
                }
                else if (!entries.empty())
                {
                    // the inotify fd has an event
                    ssize_t len = m_iNotifyWrapperPtr->read(m_inotifyFd, buf, sizeof(buf));

                    if (len == -1)
                    {
                        LOGERROR("iNotify read failed with error " << errno << ": Stopping DirectoryWatcher");
                        exit = true;
                    }
                    else if (len == 0)
                    {
                        LOGWARN("Read from inotify returned 0 bytes read");
                    }
                    else
                    {
                        // enforcing the terminate that ensure strings are null terminated.
                        // given that event-name should be null terminated anyway, there is no byte lost.
                        size_t safeDelimiter = len < static_cast<ssize_t>(sizeof(buf)) ? len : sizeof(buf)-1;
                        buf[safeDelimiter] = 0;
                        for (char* ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
                        {
                            event = (const struct inotify_event*)ptr;
                            if (event->len)
                            {
                                // event->name is null terminated and allocated in chunks of 16 bytes by iNotify
                                assert(event->len == ((std::strlen(event->name) / 16) + 1) * 16);
                                assert(event->mask == IN_MOVED_TO);
                                auto listenerMapIter = m_listenerMap.find(event->wd);
                                if (listenerMapIter != m_listenerMap.end())
                                {
                                    listenerMapIter->second->fileMoved(
                                        Common::UtilityImpl::StringUtils::checkAndConstruct(event->name));
                                }
                            }
                        }
                    }
                }
            }
            for (auto& iter : m_listenerMap)
            {
                m_iNotifyWrapperPtr->removeWatch(m_inotifyFd, iter.first);
                iter.second->watcherActive(false);
            }
            m_listenerMap.clear();
            m_watcherRunning = false;
        }
    } // namespace DirectoryWatcherImpl
} // namespace Common


namespace Common {
    namespace DirectoryWatcher {
        using Common::DirectoryWatcherImpl::DirectoryWatcher;

        IDirectoryWatcherPtr createDirectoryWatcher(IiNotifyWrapperPtr iNotifyWrapperPtr) {
            if (!iNotifyWrapperPtr)
            {
                iNotifyWrapperPtr = std::unique_ptr<IiNotifyWrapper>(new Common::DirectoryWatcherImpl::iNotifyWrapper());
            }
            return std::unique_ptr<DirectoryWatcher>(new DirectoryWatcher(std::move(iNotifyWrapperPtr)));
        }
    } // namespace DirectoryWatcher
} // namespace Common