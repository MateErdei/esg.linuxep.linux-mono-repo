/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ISessionRunner.h"
#include <pluginimpl/ApplicationPaths.h>
#include <memory>
#include <list>
#include <future>
#include <atomic>
#include <thread>


namespace sessionrunner
{
    class ParallelSessionProcessor{
    public:
        ParallelSessionProcessor(std::unique_ptr<sessionrunner::ISessionRunner> , const std::string& sessionsDirectoryPath);
        ~ParallelSessionProcessor();
        void addJob( const std::string& id);

        /*
         * This function needs to be called when the job has been done.
         * Removes the session file once the session is complete.
         * Removes session instance from currents list of running sessions.
         */
        void jobDoneCallback(const std::string& id);

    private:
        std::unique_ptr<sessionrunner::ISessionRunner> m_sessionProcessor;
        std::list<std::unique_ptr<sessionrunner::ISessionRunner>> m_runningSessions;
        std::list<std::unique_ptr<sessionrunner::ISessionRunner>> m_processedSessions;
        std::mutex m_mutex;
        std::string m_sessionsDirectoryPath;
    };

} // namespace sessionrunner
