/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IQueryProcessor.h"
#include "IResponseDispatcher.h"
#include <memory>
#include <list>
#include <future>


namespace livequery
{
    class ParallelQueryProcessor{
    public:
        ParallelQueryProcessor(std::unique_ptr<livequery::IQueryProcessor>,
            std::unique_ptr<livequery::IResponseDispatcher> );
        ~ParallelQueryProcessor();
        void addJob( const std::string& queryJson, const std::string & correlationId);
    private:
        void cleanupListOfJobs();
        std::unique_ptr<livequery::IQueryProcessor> m_queryProcessor;
        std::unique_ptr<livequery::IResponseDispatcher> m_queryDispatcher;
        std::list<std::future<void>> m_processingQueries;
    };

} // namespace livequery
