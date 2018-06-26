/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <algorithm>
#include <iostream>

#include <sys/inotify.h>
#include <unistd.h>
#include <thread>
#include "Common/DirectoryWatcher/IDirectoryWatcherException.h"
#include "Common/ZeroMQWrapper/IPoller.h"

#include "Logger.h"
#include "DirectoryWatcherImpl.h"

namespace Common
{
namespace DirectoryWatcher
{
    int iNotifyWrapper::init()
    {
        return ::inotify_init();
    }

    int iNotifyWrapper::addWatch(int fd, const char *name, uint32_t mask)
    {
        return ::inotify_add_watch(fd,  name, mask);
    }

    int iNotifyWrapper::removeWatch(int fd, int wd)
    {
        return ::inotify_rm_watch(fd, wd);
    }

    ssize_t iNotifyWrapper::read(int fd, void *buf, size_t nbytes)
    {
        return ::read(fd, buf, nbytes);
    }

    DirectoryWatcher::DirectoryWatcher(std::shared_ptr<IiNotifyWrapper> iNotifyWrapperPtr) :
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
        while (m_watcherRunning);  //Wait until thread closes
        close(m_inotifyFd);
    }

    void DirectoryWatcher::addListener(IDirectoryWatcherListener &watcherListener)
    {
        int watch = m_iNotifyWrapperPtr->addWatch(m_inotifyFd,  watcherListener.getPath().c_str(), IN_MOVED_TO);  //Only interested in files moved to the folder
        if (watch == -1)
        {
            throw IDirectoryWatcherException("Failed to add a Listener to Directory Watcher. Path: " + watcherListener.getPath());
        }
        m_listenerMap.emplace(watch, &watcherListener);
        watcherListener.watcherActive(m_watcherRunning);
    }

    void DirectoryWatcher::removeListener(IDirectoryWatcherListener &watcherListener)
    {
        auto delIter = std::find_if(m_listenerMap.begin(), m_listenerMap.end(), [&watcherListener](const std::pair<int, const IDirectoryWatcherListener*> & pair) { return pair.second == &watcherListener;});
        if (delIter != m_listenerMap.end())
        {
            m_iNotifyWrapperPtr->removeWatch(m_inotifyFd, delIter->first);
            m_listenerMap.erase(delIter);
        }
        else
        {
            throw IDirectoryWatcherException("Failed to remove a Listener from the Directory Watcher. Path: " + watcherListener.getPath());
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
        announceThreadStarted();
        m_watcherRunning = true;
        for (auto & iter : m_listenerMap)
        {
            iter.second->watcherActive(m_watcherRunning);
        }
        // Setup poller with the inotify file descriptor and a notify pipe so we are able to quit the thread.
        Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        Common::ZeroMQWrapper::IHasFDPtr iNotifyFD = poller->addEntry(m_inotifyFd, Common::ZeroMQWrapper::IPoller::POLLIN);
        Common::ZeroMQWrapper::IHasFDPtr notifyPipe = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

        bool exit = false;
        while (!exit)
        {
            char buf[4096];
            const struct inotify_event *event;
            std::vector<Common::ZeroMQWrapper::IHasFD *> resultPtr = poller->poll(std::chrono::milliseconds(-1));
            if (stopRequested())
            {
                // notifyPipe has been written. Exit the thread.
                exit = true;
            }
            else
            {
                // the inotify fd has an event
                ssize_t len = m_iNotifyWrapperPtr->read(m_inotifyFd, buf, sizeof(buf));
                if (len == -1)
                {
                    LOGERROR("iNotify read failed with error " << errno << ": Stopping DirectoryWatcher");
                    exit = true;
                }
                else
                {
                    for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
                    {
                        event = (const struct inotify_event *) ptr;

                        if (event->len)
                        {
                            auto listenerMapIter= m_listenerMap.find(event->wd);
                            if (listenerMapIter != m_listenerMap.end())
                            {
                                listenerMapIter->second->fileMoved(event->name);
                            }
                        }
                    }
                }
            }
        }
        for (auto & iter : m_listenerMap)
        {
            removeListener((*iter.second));
        }
        m_watcherRunning = false;
    }
}
}