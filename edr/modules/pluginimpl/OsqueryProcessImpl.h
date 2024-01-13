// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IOsqueryProcess.h"
#include "OsqueryStarted.h"

#include "Common/Process/IProcess.h"

#include <functional>
#include <future>

namespace Plugin
{
    class OsqueryProcessImpl : public Plugin::IOsqueryProcess
    {
    public:
        static void killAnyOtherOsquery();
        void keepOsqueryRunning(OsqueryStarted&) override;
        void requestStop() override;
        bool isRunning() override;

    private:
        void startProcess(const std::string & processPath, const std::vector<std::string>& arguments);

        Common::Process::IProcessPtr m_processMonitorPtr;
        std::mutex m_processMonitorSharedResource;
    };

    class ScopedReplaceOsqueryProcessCreator
    {
    public:
        explicit ScopedReplaceOsqueryProcessCreator(std::function<IOsqueryProcessPtr()> creator);
        ~ScopedReplaceOsqueryProcessCreator();
    };
} // namespace Plugin
