/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
    DirectoryWatcher::DirectoryWatcher()
    {
        m_watcherRunning = false;
        m_inotifyFd = inotify_init();
        if (m_inotifyFd == -1)
        {
            throw IDirectoryWatcherException("Inotify init failed.");
        }
    }

    DirectoryWatcher::~DirectoryWatcher()
    {
        close(m_inotifyFd);
    }

    void DirectoryWatcher::addWatch(std::string path, std::function<void(const std::string)> callbackFunction)
    {
        assert(!m_watcherRunning);
        int watch = inotify_add_watch(m_inotifyFd, path.c_str(), IN_MOVED_TO);  //Only interested in files moved to the folder
        if (watch == -1)
        {
            throw IDirectoryWatcherException("Failed to add a watch to inotify. Path: "+path);
        }

        DirectoryWatcherPair pair;
        pair.callback = std::move(callbackFunction);
        if (path.back() != '/')
        {
            // Ensure the path ends with a /
            path += '/';
        }
        pair.directoryPath = path;
        m_callbackMap.emplace(watch, pair);
    }

    void DirectoryWatcher::run()
    {
        announceThreadStarted();
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
                    std::cerr << "read failed" << std::endl;
                    //Check Documentation
                    return;
                }
                for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
                {
                    event = (const struct inotify_event *) ptr;

                    if (event->len)
                    {
                        //Use wd to get the full path and callback function from the map
                        DirectoryWatcherPair p = m_callbackMap[event->wd];
                        std::string eventDir = p.directoryPath;
                        p.callback(eventDir + event->name);
                    }
                }
            }
        }
    }
}
}