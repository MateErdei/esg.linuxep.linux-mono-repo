// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ConfigMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"
#include "common/SaferStrerror.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <map>
#include <iostream>
#include <utility>
#include <vector>

#include <poll.h>
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

    bool ConfigMonitor::resolveSymlinksForInterestingFiles()
    {
        auto* fs = Common::FileSystem::fileSystem();

        const auto& INTERESTING_FILES = interestingFiles();
        for (const auto& filename : INTERESTING_FILES)
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
                        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: "
                                 << common::safer_strerror(errno));
                        return false;
                    }
                    m_interestingDirs.insert_or_assign(parentDir, symlinkTargetDirFD);
                    LOGDEBUG("Monitoring "<<parentDir<<" for config changes");
                }
            }
        }
        return true;
    }

    void ConfigMonitor::run()
    {
        m_currentContents = getContentsMap();

        // Add a watch for changes to the directory base ("/etc")
        InotifyFD inotifyFD(m_base);

        bool success = resolveSymlinksForInterestingFiles();

        // Must announce before we start failing
        announceThreadStarted();

        if (inotifyFD.getFD() < 0)
        {
            LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files");
            return;
        }
        if (!success)
        {
            LOGERROR("Failed to setup inotify watches for symlink directories");
            return;
        }

        LOGDEBUG("Config Monitor entering main loop");
        while (true)
        {
            bool running = inner_run(inotifyFD);
            if (!running)
            {
                // can exit for good reasons as well as errors.
                break;
            }
            LOGDEBUG("Re-evaluating configuration symlinks");
            m_interestingDirs.clear();
            success = resolveSymlinksForInterestingFiles();
            if (!success)
            {
                LOGERROR("Failed to setup inotify watches for symlink directories");
                break;
            }
            // Now we need to check files for any changes
            // This ensures we don't miss anything while setting up our new inotify stuff
            if (check_all_files())
            {
                m_configChangedPipe.notify();
            }
            else
            {
                LOGDEBUG("System configuration not changed");
            }
        }
    }

    bool ConfigMonitor::inner_run(InotifyFD& inotifyFD)
    {
        assert(inotifyFD.getFD() >= 0);

        std::vector<struct pollfd> fds;
        fds.push_back({ .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 });
        fds.push_back({ .fd = inotifyFD.getFD(), .events = POLLIN, .revents = 0 });
        for (const auto& iter : m_interestingDirs)
        {
            fds.push_back({ .fd = iter.second->getFD(), .events = POLLIN, .revents = 0 });
        }

        while (true)
        {
            auto ret = m_sysCalls->ppoll(&fds[0], std::size(fds), nullptr, nullptr);
            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
                return false;
            }
            assert(ret > 0);

            if (stopRequested())
            {
                return false;
            }

            if ((fds[0].revents & POLLERR) != 0)
            {
                LOGERROR("Closing Config Monitor, error from notify pipe");
                return false;
            }

            if ((fds[1].revents & POLLERR) != 0)
            {
                LOGERROR("Closing Config Monitor, error from inotify");
                return false;
            }

            if ((fds[0].revents & POLLIN) != 0)
            {
                LOGDEBUG("Closing Config Monitor");
                return false;
            }

            bool interestingDirTouched = false;
            if ((fds[1].revents & POLLIN) != 0)
            {
                // Something changed under m_base (/etc)
                // Per https://man7.org/linux/man-pages/man7/inotify.7.html
                // This buffer should be aligned for struct inotify_event
                char buffer[EVENT_BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));

                ssize_t length = ::read(inotifyFD.getFD(), buffer, EVENT_BUF_LEN);

                if (length < 0)
                {
                    LOGERROR("Failed to read event from inotify: " << common::safer_strerror(errno));
                    return false;
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
                            interestingDirTouched = true;
                        }
                    }
                    i += EVENT_SIZE + event->len;
                }
            }

            for (unsigned long i = 2; i < fds.size(); i++)
            {
                if ((fds[i].revents & POLLIN) != 0)
                {
                    // flush the buffer
                    char buffer[EVENT_BUF_LEN];
                    ::read(fds[i].fd, buffer, EVENT_BUF_LEN);

                    LOGDEBUG("Inotified in directory");
                    interestingDirTouched = true;
                }
            }

            if (interestingDirTouched)
            {
                LOGDEBUG("Change detected in monitored directory, checking config for changes");
                return true;
            }
        }
    }

    bool ConfigMonitor::check_file_for_changes(const std::string& filepath, bool logUnchanged)
    {
        auto newContents = getContents(filepath);
        if (m_currentContents.at(filepath) != newContents)
        {
            LOGINFO("System configuration updated for " << filepath);
            LOGDEBUG("Old content size=" << m_currentContents.at(filepath).size());
            LOGDEBUG("New content size=" << newContents.size());
            m_currentContents[filepath] = newContents;
            return true;
        }
        else if (logUnchanged)
        {
            LOGINFO("System configuration not changed for " << filepath);
        }

        return false;
    }

    bool ConfigMonitor::check_all_files()
    {
        bool configChanged = false;
        for (const auto& filepath : interestingFiles())
        {
            // Don't short-circuit - we want to update all contents in case of simultaneous updates
            configChanged = check_file_for_changes(filepath, false) || configChanged;
        }
        return configChanged;
    }
}
