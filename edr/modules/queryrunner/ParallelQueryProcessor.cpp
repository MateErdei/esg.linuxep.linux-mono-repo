// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ParallelQueryProcessor.h"
#include "Logger.h"

namespace queryrunner{

    ParallelQueryProcessor::ParallelQueryProcessor(std::unique_ptr<queryrunner::IQueryRunner> queryProcessor):
                                                   m_queryProcessor{std::move(queryProcessor)}
    {

    }

    ParallelQueryProcessor::~ParallelQueryProcessor()
    {
        abortQueries();
    }

    void ParallelQueryProcessor::newJobDone(const std::string& id)
    {
        try
        {
            std::lock_guard<std::mutex> l { m_mutex };
            LOGDEBUG("Query " << id << " finished");

            // clearing up previous queries objects that now can be removed as they are not in the same thread.
            m_processedQueries.clear();

            // this method retrieves the result and mark the queryRunner to be cleared after as it can not be removed as this method is executed in the queryRunner callback thread.

            auto it = std::find_if(
                m_processingQueries.begin(),
                m_processingQueries.end(),
                [id](const std::unique_ptr<queryrunner::IQueryRunner>& irunner) { return irunner->id() == id; });

            if (it != m_processingQueries.end())
            {
                auto result = it->get()->getResult();
                m_telemetry.processLiveQueryResponseStats(result);
                LOGDEBUG("One more entry removed from the queue of processing queries");
                // move this element to the other list to clear it afterwards.
                // it cannot be clear here as this is executed in the callback of the queryrunnerthread.
                m_processedQueries.splice(m_processedQueries.begin(), m_processingQueries, it);
            }
            else
            {
                LOGWARN("Failed to find the query " << id << " in the list of processing queries");
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failed to handle feedback on query finished: " << ex.what());
        }
        
        if (m_shuttingDown)
        {
            abortQueries();
        }
    }

    void ParallelQueryProcessor::addJob(const std::string &queryJson, const std::string &correlationId) 
    {
        if (!m_shuttingDown)
        {
            try
            {
                auto newproc = m_queryProcessor->clone();
                std::lock_guard<std::mutex> l { m_mutex };
                newproc->triggerQuery(correlationId, queryJson,
                          [this](const std::string& id) { this->newJobDone(id); });
                m_processingQueries.emplace_back(std::move(newproc));
            }
            catch (const std::exception& ex)
            {
                LOGERROR("Failed to configure a new query: " << ex.what());
            }
        }
    }

    void ParallelQueryProcessor::abortQueries()
    {
        m_shuttingDown = true;
        try
        {
            std::vector<std::thread> shutdownThreads;
            {
                std::lock_guard<std::mutex> l{m_mutex};
                for (auto& p : m_processingQueries)
                {
                    shutdownThreads.emplace_back(&IQueryRunner::requestAbort, p.get());
                }
            }
            for (auto& shutdownThread : shutdownThreads)
            {
                if (shutdownThread.joinable())
                {
                    shutdownThread.join();
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failure in clean up ParallelQueryProcessor: " << ex.what());
        }
    }
}
