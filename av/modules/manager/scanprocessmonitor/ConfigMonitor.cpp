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
                        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: " << strerror(errno));
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
            // Now we need to check files for any changes - in case we missed then when we
            // didn't monitor directories
            if (check_all_files())
            {
                m_configChangedPipe.notify();
            }
        }
    }

    bool ConfigMonitor::inner_run(InotifyFD& inotifyFD)
    {
        assert(inotifyFD.getFD() >= 0);

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
                return false;
            }

            if (active < 0 and errno != EINTR)
            {
                LOGERROR("failure in ConfigMonitor: pselect failed: " << strerror(errno));
                return false;
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
                            configChanged = check_file_for_changes(event->name, true) || configChanged;
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
                if (check_all_files())
                {
                    configChanged = true;
                }
            }

            if (configChanged)
            {
                // Only notify if the actual contents have changed
                m_configChangedPipe.notify();
            }

            if (interestingDirTouched)
            {
                // Need to re-evaluate directories
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
