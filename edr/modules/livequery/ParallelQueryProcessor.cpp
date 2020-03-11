/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ParallelQueryProcessor.h"
#include "Logger.h"

namespace
{
    void processQueryInThread(std::string queryJson, std::string correlationId, std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
                      std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher)
    {
        livequery::processQuery(*queryProcessor, *responseDispatcher, queryJson, correlationId);
    }
}

namespace livequery{

    ParallelQueryProcessor::ParallelQueryProcessor(std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
                                                   std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher):
                                                   m_queryProcessor{std::move(queryProcessor)},
                                                   m_queryDispatcher{std::move(responseDispatcher)}
    {

    }

    ParallelQueryProcessor::~ParallelQueryProcessor()
    {
        for( auto & l: m_processingQueries)
        {
            l.wait();
        }
    }

    void ParallelQueryProcessor::addJob(const std::string &queryJson, const std::string &correlationId) {
        m_processingQueries.emplace_back( std::async(std::launch::async, [this, queryJson, correlationId]()
        {
            processQueryInThread(queryJson, correlationId, m_queryProcessor->clone(), m_queryDispatcher->clone());
        })  );
        cleanupListOfJobs();
    }

    void ParallelQueryProcessor::cleanupListOfJobs()
    {
        for( auto it = m_processingQueries.begin(); it != m_processingQueries.end(); )
        {
            if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                LOGDEBUG("One more entry removed from the queue of processing queries");
                it = m_processingQueries.erase(it);
            } else{
                it++;
            }
        }
    }
}
