/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"

//#include "Common/FileSystem/IFileSystem.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <map>
#include <iostream>

#include <sys/inotify.h>
#include <sys/types.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define MAX_SYMLINK_DEPTH  10

using namespace plugin::manager::scanprocessmonitor;

static const std::vector<std::string>& interestingFiles()
{
    static const std::vector<std::string> INTERESTING_FILES
        {
            "nsswitch.conf",
            "resolv.conf",
            "ld.so.cache",
            "host.conf",
            "hosts",
        };
    return INTERESTING_FILES;
}

static std::vector<fs::path> interestingFilePaths(fs::path dir)
{
    std::vector<fs::path> interestingFilePaths;
    const auto& INTERESTING_FILES = interestingFiles();
    for (const auto& filename : INTERESTING_FILES)
    {
        interestingFilePaths.emplace_back(dir / filename);
    }
    return interestingFilePaths;
}

static inline bool isInteresting(const std::string& basename)
{
//    LOGDEBUG("isInteresting:"<<basename);
    const auto& INTERESTING_FILES = interestingFiles();
    return (std::find(INTERESTING_FILES.begin(), INTERESTING_FILES.end(), basename) != INTERESTING_FILES.end());
}

ConfigMonitor::ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                             std::string base)
    : m_configChangedPipe(pipe)
    , m_base(std::move(base))
    , m_interestingFiles(interestingFilePaths(m_base))
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
    std::string filepath = m_base + "/" + basename;
    std::ifstream in(filepath, std::ios::in|std::ios::binary);
    if (!in.is_open())
    {
        return ""; // Treat missing as empty
    }
    return {(std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()};
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
    fs::path origCwd = fs->currentWorkingDirectory();
    int ret = ::chdir(m_base.c_str());
    if (ret != 0)
    {
        LOGWARN("Failed to change working directory to " << m_base << " to get symlink target (" << ret << ")");
    }

    for (auto& filepath: m_interestingFiles)
    {
        // resolve symlinked config files up to a depth of MAX_SYMLINK_DEPTH
        int count = 0;
        while (count++ < MAX_SYMLINK_DEPTH)
        {
            std::optional<Path> optPath = fs->readlink(filepath);
            if (optPath.has_value())
            {
                fs::path symlinkTarget = fs::canonical(optPath.value());
                fs::path parentDir = symlinkTarget.parent_path();
                ret = ::chdir(parentDir.c_str());
                if (ret != 0)
                {
                    LOGWARN("Failed to change working directory to " << parentDir << " to get symlink target (" << ret << ")");
                }
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
                filepath = symlinkTarget;
            }
            else
            {
                break;
            }
        }
    }

    ret = ::chdir(origCwd.c_str());
    if (ret != 0)
    {
        LOGWARN("Failed to restore working directory to " << origCwd << " (" << ret << ")");
    }
}

void ConfigMonitor::run()
{
    auto contents = getContentsMap();

    // Add a watch for changes to the directory base ("/etc")
    InotifyFD inotifyFD(m_base);
    if (inotifyFD.getFD() < 0)
    {
        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: " << strerror(errno));
        return;
    }

    // Only announce we've started once the watch is setup.
    announceThreadStarted();

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    max_fd = FDUtils::addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, inotifyFD.getFD(), max_fd);
    for (const auto& iter: m_interestingDirs)
    {
        max_fd = FDUtils::addFD(&readfds, iter.second->getFD(), max_fd);
    }

    while(true)
    {
        fd_set temp_readfds = readfds;
        int active = ::pselect(max_fd+1, &temp_readfds, nullptr, nullptr, nullptr, nullptr);

        if (active < 0 and errno != EINTR)
        {
            LOGERROR("failure in ConfigMonitor: pselect failed: " << strerror(errno));
            break;
        }

        if (stopRequested())
        {
            break;
        }

        if (FDUtils::fd_isset(inotifyFD.getFD(), &temp_readfds))
        {
//            LOGDEBUG("inotified");
            // Something changed under m_base (/etc)
            char buffer[EVENT_BUF_LEN];
            ssize_t length = ::read( inotifyFD.getFD(), buffer, EVENT_BUF_LEN );

            if (length < 0)
            {
                LOGERROR("Failed to read event from inotify: "<< strerror(errno));
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
                            LOGINFO("System configuration not changed for "<< event->name);
                        }
                        else
                        {
                            LOGINFO("System configuration updated for " << event->name);
                            m_configChangedPipe.notify();
                            LOGDEBUG("Old content size=" << contents.at(event->name).size());
                            LOGDEBUG("New content size=" << newContents.size());
                            contents[event->name] = newContents;
                        }
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }

        for (const auto& iter: m_interestingDirs)
        {
            if (FDUtils::fd_isset(iter.second->getFD(), &temp_readfds))
            {
                for (auto& filepath: interestingFiles())
                {
                    auto newContents = getContents(filepath);
                    if (contents.at(filepath) != newContents)
                    {
                        LOGINFO("System configuration updated for " << filepath);
                        m_configChangedPipe.notify();
                        LOGDEBUG("Old content size=" << contents.at(filepath).size());
                        LOGDEBUG("New content size=" << newContents.size());
                        contents[filepath] = newContents;
                    }
                }
            }
        }
    }
}