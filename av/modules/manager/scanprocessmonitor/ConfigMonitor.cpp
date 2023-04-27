// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ConfigMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"
#include "common/SaferStrerror.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

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
        constexpr const char* SOPHOS_PROXY_PATH = "SOPHOS_PROXY_PATH";

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
        std::string base,
        std::string proxyConfigFile) :
        m_configChangedPipe(pipe),
        m_base(std::move(base)),
        proxyConfigFile_(std::move(proxyConfigFile)),
        m_sysCalls(std::move(systemCallWrapper))
    {
        if (inotifyFd_.getFD() < 0)
        {
            LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files");
            throw std::runtime_error("Unable to initialise inotify");
        }

        if (proxyConfigFile_ == "DEFAULT")
        {
            proxyConfigFile_ = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        }
    }

    std::string ConfigMonitor::getContentsFromPath(const fs::path& filepath)
    {
        if (filepath.empty())
        {
            return ""; // Threat empty path as empty file
        }
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (!in.is_open())
        {
            return ""; // Treat missing as empty
        }
        return { (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>() };
    }

    /**
     * We assume all the files monitored as small, so we can just keep a record of them.
     * @param filepath
     * @return
     */
    std::string ConfigMonitor::getContents(const std::string& basename)
    {
        std::string filepath = m_base / basename;
        if (basename == SOPHOS_PROXY_PATH)
        {
            filepath = proxyConfigFile_;
        }
        return getContentsFromPath(filepath);
    }

    ConfigMonitor::contentMap_t ConfigMonitor::getContentsMap()
    {
        const auto& INTERESTING_FILES = interestingFiles();
        contentMap_t result;
        for (const auto& basename : INTERESTING_FILES)
        {
            result[basename] = getContents(basename);
        }
        result[SOPHOS_PROXY_PATH] = getContents(SOPHOS_PROXY_PATH);
        return result;
    }

    static constexpr auto INOTIFY_MASK = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE;

    bool ConfigMonitor::resolveSymlinksForInterestingFiles()
    {
        auto* fs = Common::FileSystem::fileSystem();

        // Remove existing watches
        for (auto const& [path, wd] : m_interestingDirs)
        {
            inotifyFd_.unwatch(wd);
        }

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
                    int wd = inotifyFd_.watch(parentDir, INOTIFY_MASK);
                    if (wd < 0)
                    {
                        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: "
                                 << common::safer_strerror(errno));
                        return false;
                    }
                    m_interestingDirs.insert_or_assign(parentDir, wd);
                    LOGDEBUG("Monitoring "<<parentDir<<" for config changes");
                }
            }
        }
        return true;
    }

    void ConfigMonitor::run()
    {
        m_currentContents = getContentsMap();

        bool success = true;

        // Add a watch for changes to the directory base ("/etc")
        etcWatch_ = inotifyFd_.watch(m_base, INOTIFY_MASK);
        if (etcWatch_ < 0)
        {
            LOGERROR("Failed to watch directory: "
                     << m_base << " - Unable to monitor DNS config files: " << common::safer_strerror(errno));
            LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files");
            success = false;
        }
        if (!proxyConfigFile_.empty())
        {
            proxyConfigWatch_ = inotifyFd_.watch(proxyConfigFile_, INOTIFY_MASK);
            if (proxyConfigWatch_ < 0)
            {
                LOGERROR("Failed to watch proxy configuration file");
            }
        }

        if (!resolveSymlinksForInterestingFiles())
        {
            LOGERROR("Failed to initialise inotify: Unable to monitor DNS symlink locations");
            success = false;
        }

        // Must get the original contents before we announce the thread has started
        // And mark our watches
        announceThreadStarted();

        if (!success)
        {
            LOGERROR("Failed to setup inotify watches for system and proxy files");
            return;
        }

        LOGDEBUG("Config Monitor entering main loop");
        while (true)
        {
            bool running = inner_run();
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

    bool ConfigMonitor::inner_run()
    {
        // Per https://man7.org/linux/man-pages/man7/inotify.7.html
        // This buffer should be aligned for struct inotify_event
        char buffer[EVENT_BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));

        assert(inotifyFd_.getFD() >= 0);

        constexpr int num_fds = 2;
        struct pollfd fds[num_fds];

        fds[0].fd = m_notifyPipe.readFd();
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        fds[1].fd = inotifyFd_.getFD();
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        while (true)
        {
            int activity = m_sysCalls->ppoll(fds, num_fds, nullptr, nullptr);

            if (stopRequested() || (fds[0].revents & POLLIN) != 0)
            {
                return false;
            }

            if (activity < 0 and errno != EINTR)
            {
                LOGERROR("failure in ConfigMonitor: ppoll failed: " << common::safer_strerror(errno));
                return false;
            }

            bool interestingDirTouched = false;
            if ((fds[1].revents & POLLIN) != 0)
            {
                // Something changed under our watches
                ssize_t length = ::read(inotifyFd_.getFD(), buffer, EVENT_BUF_LEN);

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
                        if (event->wd == etcWatch_)
                        {
                            // For etc actually check the filename, since we know the list
                            // Don't care if it's close-after-write or rename/move
                            if (isInteresting(event->name))
                            {
                                interestingDirTouched = true;
                                break;
                            }
                        }
                        else
                        {
                            if (event->wd == proxyConfigWatch_)
                            {
                                LOGDEBUG("Proxy configuration changed");
                            }
                            interestingDirTouched = true;
                            break;
                        }
                    }
                    i += EVENT_SIZE + event->len;
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
        configChanged = check_file_for_changes(SOPHOS_PROXY_PATH, false) || configChanged;

        return configChanged;
    }
}
