/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <algorithm>
#include <iostream>

#include <sys/inotify.h>
#include <unistd.h>
#include <thread>
#include <cassert>
#include <Common/DirectoryWatcher/IDirectoryWatcherException.h>
#include "Common/ZeroMQWrapper/IPoller.h"

#include "DirectoryWatcherImpl.h"

namespace Common
{
namespace DirectoryWatcher
{
    int iNotifyWrapper::init()
    {
        return ::inotify_init();
    }

    int iNotifyWrapper::add_watch(int __fd, const char *__name, uint32_t __mask)
    {
        return ::inotify_add_watch(__fd,  __name, __mask);
    }


    DirectoryWatcher::DirectoryWatcher(std::shared_ptr<IiNotifyWrapper> iNotifyWrapperPtr) :
    m_iNotifyWrapperPtr(std::move(iNotifyWrapperPtr))
    {
        m_watcherRunning = false;
        m_inotifyFd = m_iNotifyWrapperPtr->init();
        if (m_inotifyFd == -1)
        {
            throw IDirectoryWatcherException("Inotify init failed.");
        }
    }

    DirectoryWatcher::~DirectoryWatcher()
    {
        endWatch();
        close(m_inotifyFd);
    }

    void DirectoryWatcher::addListener(IDirectoryWatcherListener &watcherListener)
    {
        assert(!m_watcherRunning);
        int watch = m_iNotifyWrapperPtr->add_watch(m_inotifyFd,  watcherListener.getPath().c_str(), IN_MOVED_TO);  //Only interested in files moved to the folder
        if (watch == -1)
        {
            throw IDirectoryWatcherException("Failed to add a watch to inotify. Path: " + watcherListener.getPath());
        }
        m_listenerMap.emplace(watch, &watcherListener);
    }

    void DirectoryWatcher::removeListener(IDirectoryWatcherListener &watcherListener)
    {
        auto delIter = std::find_if(m_listenerMap.begin(), m_listenerMap.end(), [&watcherListener](const std::pair<int, const IDirectoryWatcherListener*> & pair) { return pair.second == &watcherListener;});
        if (delIter != m_listenerMap.end())
        {
            m_listenerMap.erase(delIter);
        }
        else
        {
            std::cerr << "Can't remove Listener to path " <<  watcherListener.getPath() << std::endl;
        }
    }

    void DirectoryWatcher::startWatch()
    {
        start();
    }

    void DirectoryWatcher::endWatch()
    {
        requestStop();
        while (m_watcherRunning);  //Wait until thread closes
    }

    void DirectoryWatcher::run()
    {
        announceThreadStarted();
        assert(!m_listenerMap.empty());
        m_watcherRunning = true;
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
                ssize_t len = read(m_inotifyFd, buf, sizeof(buf));
                if (len == -1)
                {
                    std::cerr << "iNotify read failed with error " << errno << ": Stopping DirectoryWatcher" << std::endl;
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
                                listenerMapIter->second->fileAdded(event->name);
                            }
                        }
                    }
                }
            }
        }
        for (auto & iter : m_listenerMap)
        {
            iter.second->watcherClosed();
            std::cout << "Delete in thread" << std::endl;
        }
        m_listenerMap.clear();
        m_watcherRunning = false;
    }
}
}

