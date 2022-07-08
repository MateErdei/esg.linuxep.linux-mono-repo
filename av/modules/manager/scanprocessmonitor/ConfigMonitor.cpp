/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <map>
#include <iostream>
#include <utility>
#include <vector>

#include <sys/inotify.h>
#include <sys/types.h>


namespace plugin::manager::scanprocessmonitor
{
    namespace
    {
        constexpr size_t EVENT_SIZE = sizeof (struct inotify_event);
        constexpr size_t EVENT_BUF_LEN = 1024 * ( EVENT_SIZE + 16 );
        constexpr int MAX_SYMLINK_DEPTH = 10;

        const std::vector<std::string>& interestingFiles()
        {
            static const std::vector<std::string> INTERESTING_FILES {
                "nsswitch.conf",
                "resolv.conf",
                "ld.so.cache",
                "host.conf",
                "hosts",
            };
            return INTERESTING_FILES;
        }

        inline bool isInteresting(const std::string& basename)
        {
            //    LOGDEBUG("isInteresting:"<<basename);
            const auto& INTERESTING_FILES = interestingFiles();
            return (std::find(INTERESTING_FILES.begin(), INTERESTING_FILES.end(), basename) != INTERESTING_FILES.end());
        }
    }

    ConfigMonitor::ConfigMonitor(
        Common::Threads::NotifyPipe& pipe,
        datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
        std::string base) :
        m_configChangedPipe(pipe),
        m_base(std::move(base)),
        m_sysCalls(std::move(systemCallWrapper))
    {
        resolveSymlinksForInterestingFiles();
    }

    /**
     * We assume all the files monitored as small, so we can just keep a record of them.
     * @param filepath
     * @return
     */
    std::string ConfigMonitor::getContents(const std::string& basename)
    {
        std::string filepath = m_base / basename;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (!in.is_open())
        {
            return ""; // Treat missing as empty
        }
        return { (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>() };
    }

    ConfigMonitor::contentMap_t ConfigMonitor::getContentsMap()
    {
        const auto& INTERESTING_FILES = interestingFiles();
        contentMap_t result;
        for (const auto& basename : INTERESTING_FILES)
        {
            result[basename] = getContents(basename);
        }
        return result;
    }

    void ConfigMonitor::resolveSymlinksForInterestingFiles()
    {
        auto fs = Common::FileSystem::fileSystem();

        const auto& INTERESTING_FILES = interestingFiles();
        for (auto& filename : INTERESTING_FILES)
        {
            // resolve symlinked config files up to a depth of MAX_SYMLINK_DEPTH
            fs::path filepath = m_base / filename;
            for (int count = 0; count < MAX_SYMLINK_DEPTH; ++count)
            {
                std::optional<Path> optPath = fs->readlink(filepath);
                if (!optPath.has_value())
                {
                    break;
                }

                filepath = optPath.value();
                fs::path parentDir = filepath.parent_path();
                if (m_interestingDirs.find(parentDir) == m_interestingDirs.end())
                {
                    // record directory if it has not already been recorded
                    auto symlinkTargetDirFD = std::make_shared<InotifyFD>(parentDir);
                    if (symlinkTargetDirFD->getFD() < 0)
                    {
                        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: " << strerror(errno));
                        return;
                    }
                    m_interestingDirs.insert_or_assign(parentDir, symlinkTargetDirFD);
                }
            }
        }
    }

    void ConfigMonitor::run()
    {
        auto contents = getContentsMap();

        // Add a watch for changes to the directory base ("/etc")
        InotifyFD inotifyFD(m_base);

        // do this after trying to set up watches, but before any possible return due to errors.
        announceThreadStarted();

        if (inotifyFD.getFD() < 0)
        {
            LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files");
            return;
        }

        fd_set readFds;
        FD_ZERO(&readFds);
        int max_fd = -1;
        max_fd = FDUtils::addFD(&readFds, m_notifyPipe.readFd(), max_fd);
        max_fd = FDUtils::addFD(&readFds, inotifyFD.getFD(), max_fd);
        for (const auto& iter : m_interestingDirs)
        {
            max_fd = FDUtils::addFD(&readFds, iter.second->getFD(), max_fd);
        }

        while (true)
        {
            bool configChanged = false;
            fd_set temp_readFds = readFds;
            int active = m_sysCalls->pselect(max_fd + 1, &temp_readFds, nullptr, nullptr, nullptr, nullptr);

            if (stopRequested())
            {
                break;
            }

            if (active < 0 and errno != EINTR)
            {
                LOGERROR("failure in ConfigMonitor: pselect failed: " << strerror(errno));
                break;
            }

            if (FDUtils::fd_isset(inotifyFD.getFD(), &temp_readFds))
            {
                //            LOGDEBUG("inotified");
                // Something changed under m_base (/etc)
                char buffer[EVENT_BUF_LEN];
                ssize_t length = ::read(inotifyFD.getFD(), buffer, EVENT_BUF_LEN);

                if (length < 0)
                {
                    LOGERROR("Failed to read event from inotify: " << strerror(errno));
                    break;
                }

                uint32_t i = 0;
                while (i < length)
                {
                    auto* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
                    if (event->len)
                    {
                        // Don't care if it's close-after-write or rename/move
                        if (isInteresting(event->name))
                        {
                            auto newContents = getContents(event->name);
                            if (contents.at(event->name) == newContents)
                            {
                                LOGINFO("System configuration not changed for " << event->name);
                            }
                            else
                            {
                                LOGINFO("System configuration updated for " << event->name);
                                configChanged = true;
                                LOGDEBUG("Old content size=" << contents.at(event->name).size());
                                LOGDEBUG("New content size=" << newContents.size());
                                contents[event->name] = newContents;
                            }
                        }
                    }
                    i += EVENT_SIZE + event->len;
                }
            }

            bool interestingDirTouched = false;
            for (const auto& iter : m_interestingDirs)
            {
                int fd = iter.second->getFD();
                if (FDUtils::fd_isset(fd, &temp_readFds))
                {
                    // flush the buffer
                    char buffer[EVENT_BUF_LEN];
                    ::read(fd, buffer, EVENT_BUF_LEN);

                    LOGDEBUG("Inotified in directory: " << iter.first);
                    interestingDirTouched = true;
                }
            }

            if (interestingDirTouched)
            {
                LOGDEBUG("Change detected in monitored directory, checking config for changes");
                for (auto& filepath : interestingFiles())
                {
                    auto newContents = getContents(filepath);
                    if (contents.at(filepath) != newContents)
                    {
                        LOGINFO("System configuration updated for " << filepath);
                        configChanged = true;
                        LOGDEBUG("Old content size=" << contents.at(filepath).size());
                        LOGDEBUG("New content size=" << newContents.size());
                        contents[filepath] = newContents;
                    }
                }
            }

            if (configChanged)
            {
                m_configChangedPipe.notify();
            }
        }
    }
}