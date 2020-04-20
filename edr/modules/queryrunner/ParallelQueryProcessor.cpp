/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ParallelQueryProcessor.h"
#include "Logger.h"

namespace queryrunner{

    ParallelQueryProcessor::ParallelQueryProcessor(std::unique_ptr<queryrunner::IQueryRunner> queryProcessor):
                                                   m_queryProcessor{std::move(queryProcessor)}, m_ignoreCallBack{false}
    {

    }

    ParallelQueryProcessor::~ParallelQueryProcessor()
    {
        m_ignoreCallBack = true;    
        std::lock_guard<std::mutex> l{m_mutex};             
        for( auto & l: m_processingQueries)
        {
            auto result = l->getResult(); 
            m_telemetry.processLiveQueryResponseStats(result); 
        }
    }

    void ParallelQueryProcessor::newJobDone(std::string id)
    {
        if ( m_ignoreCallBack) return;
        std::lock_guard<std::mutex> l{m_mutex};
        for( auto it = m_processingQueries.begin(); it != m_processingQueries.end(); )
        {
            if (it->get()->id() == id)
            {
                auto result = it->get()->getResult(); 
                m_telemetry.processLiveQueryResponseStats(result); 
                LOGSUPPORT("One more entry removed from the queue of processing queries");
                it = m_processingQueries.erase(it);
                return; 
            } else{
                it++;
            }
        }
        LOGWARN("Failed to find the job in the list of managed jobs: " << id ); 
    }

    void ParallelQueryProcessor::addJob(const std::string &queryJson, const std::string &correlationId) {

        auto newproc = m_queryProcessor->clone(); 
        newproc->triggerQuery( queryJson, correlationId, [this](std::string id){this->newJobDone(id);});
        std::lock_guard<std::mutex> l{m_mutex};
        m_processingQueries.emplace_back(std::move(newproc)); 
    }
}
