/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IOsqueryProcess.h"

#include <Common/Process/IProcess.h>

#include <functional>
#include <future>

namespace Plugin
{
    class OsqueryProcessImpl : public Plugin::IOsqueryProcess
    {
    public:
        static void killAnyOtherOsquery();
        OsqueryProcessImpl();
        ~OsqueryProcessImpl();
        void keepOsqueryRunning() override;
        void requestStop() override;

    private:
        void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath);
        void regenerateOSQueryConfigFile(const std::string& osqueryConfigFilePath);

        void startProcess(const std::string & processPath, const std::vector<std::string>& arguments);

        Common::Process::IProcessPtr m_processMonitorPtr;
        std::mutex m_processMonitorSharedResource;
        std::atomic<bool> m_safeToDestroy;
    };

    class ScopedReplaceOsqueryProcessCreator
    {
    public:
        ScopedReplaceOsqueryProcessCreator(std::function<IOsqueryProcessPtr()> creator);
        ~ScopedReplaceOsqueryProcessCreator();
    };
} // namespace Plugin
