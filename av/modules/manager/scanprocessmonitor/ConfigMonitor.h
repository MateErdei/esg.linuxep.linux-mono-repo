/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "InotifyFD.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/ISystemCallWrapper.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/Threads/AbstractThread.h"

#include <map>
#include <vector>

namespace fs = sophos_filesystem;

namespace plugin::manager::scanprocessmonitor
{
    class ConfigMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                               datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
                               std::string base="/etc");

    private:
        using contentMap_t = std::map<std::string, std::string>;
        void run() override;
        void resolveSymlinksForInterestingFiles();

        std::string getContents(const std::string& basename);
        contentMap_t getContentsMap();

        Common::Threads::NotifyPipe& m_configChangedPipe;
        fs::path m_base;

        std::map<fs::path, std::shared_ptr<InotifyFD>> m_interestingDirs;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
    };
}
