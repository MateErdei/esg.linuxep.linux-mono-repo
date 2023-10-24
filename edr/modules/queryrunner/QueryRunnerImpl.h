// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IQueryRunner.h"

#include "Common/Process/IProcess.h"

#include <future>
namespace queryrunner
{
    class QueryRunnerImpl: public IQueryRunner
    {
        std::string m_osquerySocketPath; 
        std::string m_executablePath;
        Common::Process::IProcessPtr m_process; 
        std::string m_correlationId; 
        std::future<void> m_fut;
        std::mutex m_mutex; 
        QueryRunnerStatus m_runnerStatus;  
    public:
        static void setStatusFromExitResult( QueryRunnerStatus& queryrunnerStatus, int exitCode, const std::string & output); 
        QueryRunnerImpl(const std::string &osquerySocketPath, const std::string &executablePath);
        ~QueryRunnerImpl() {};
        void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) override;
        virtual std::string id() override; 
        virtual QueryRunnerStatus getResult() override; 
        void requestAbort() override; 
        virtual std::unique_ptr<IQueryRunner> clone() override;
    private:
        void setStatus(int exitCode, const std::string& output);
    };
}
