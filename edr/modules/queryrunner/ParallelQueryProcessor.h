/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IQueryRunner.h"
#include "Telemetry.h"
#include <memory>
#include <list>
#include <future>
#include <atomic>


namespace queryrunner
{
    class ParallelQueryProcessor{
    public:
        ParallelQueryProcessor(std::unique_ptr<queryrunner::IQueryRunner> );
        ~ParallelQueryProcessor();
        void addJob( const std::string& queryJson, const std::string & correlationId);
        void newJobDone(std::string id); 

    private:
        void cleanupListOfJobs(); 
        std::unique_ptr<queryrunner::IQueryRunner> m_queryProcessor;
        queryrunner::Telemetry m_telemetry; 
        std::atomic<bool> m_ignoreCallBack;
        std::list<std::unique_ptr<queryrunner::IQueryRunner>> m_processingQueries;
        std::mutex m_mutex; 
    };

} // namespace queryrunner
