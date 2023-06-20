// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/AbstractThreadPluginInterface.h"
#include "common/MultiInotifyFD.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/SystemCallWrapper/ISystemCallWrapper.h"
#include "Common/Threads/NotifyPipe.h"

#include <map>

namespace plugin::manager::scanprocessmonitor
{
    namespace fs = sophos_filesystem;
    class ConfigMonitor : public common::AbstractThreadPluginInterface
    {
    public:

        using InotifyFD_t = common::MultiInotifyFD;
        /**
         * Thread that monitors for DNS config changes and proxy config changes
         *
         * @param pipe
         * @param systemCallWrapper
         * @param base Optional base of the config files to monitor
         * @param proxyConfigDirectory Optional path to the proxy config file's parent directory to monitor - defaults to $SOPHOS_INSTALL/base/etc/sophosspl
         */
        explicit ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                               Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper,
                               std::string base="/etc",
                               std::string proxyConfigDirectory="DEFAULT");

    private:
        using contentMap_t = std::map<std::string, std::string>;
        void run() override;

        /**
         * @return True if we successfully setup symlinks
         */
        bool resolveSymlinksForInterestingFiles();

        /**
         *
         * @return True if we should continue running
         */
        bool inner_run();

        /**
         *
         * @param filepath
         * @return True if the file has changed contents
         */
        bool check_file_for_changes(const std::string& filepath, bool logUnchanged);

        /**
         * Check all files for changes
         * @return True if any of the interesting files have changed contents
         */
        bool check_all_files();

        std::string getContents(const std::string& basename);
        static std::string getContentsFromPath(const fs::path& path);
        contentMap_t getContentsMap();

        Common::Threads::NotifyPipe& m_configChangedPipe;
        fs::path m_base;
        fs::path proxyConfigParentDirectory_;
        std::string proxyConfigFileName_;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCalls;

        /**
         * Map of watched paths to watchFD on inotifyFd_
         */
        std::map<fs::path, int> m_interestingDirs;
        contentMap_t m_currentContents;

        common::MultiInotifyFD inotifyFd_;
        int etcWatch_ = -1;
        int proxyConfigWatch_ = -1;

    };
}
