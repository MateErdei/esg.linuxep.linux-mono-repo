/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "InotifyFD.h"

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/ISystemCallWrapper.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Threads/NotifyPipe.h"

#include <map>

namespace plugin::manager::scanprocessmonitor
{
    namespace fs = sophos_filesystem;
    class ConfigMonitor : public common::AbstractThreadPluginInterface
    {
    public:

        using InotifyFD_t = InotifyFD;
        /**
         *
         * @param pipe
         * @param systemCallWrapper
         * @param base Optional base of the config files to monitor
         * @param proxyConfigFile Optional path to the proxy config file to monitor - defaults to $SOPHOS_INSTALL/
         */
        explicit ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                               datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
                               std::string base="/etc",
                               std::string proxyConfigFile="DEFAULT");

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
        bool inner_run(InotifyFD_t& inotifyFD, InotifyFD_t& proxyWatcher);

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
        fs::path proxyConfigFile_;

        std::map<fs::path, std::shared_ptr<InotifyFD_t>> m_interestingDirs;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        contentMap_t m_currentContents;

    };
}
