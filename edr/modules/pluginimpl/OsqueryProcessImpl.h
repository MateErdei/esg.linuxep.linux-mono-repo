/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IOsqueryProcess.h"

#include <Common/Process/IProcess.h>
#include "OsqueryStarted.h"
#include <functional>
#include <future>

namespace Plugin
{
    void processOsqueryLogLineForTelemetry(std::string &logLine);
    void ingestOutput(const std::string& output);

    class OsqueryProcessImpl : public Plugin::IOsqueryProcess
    {
    public:
        static void killAnyOtherOsquery();
        void keepOsqueryRunning(OsqueryStarted&) override;
        void requestStop() override;

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
