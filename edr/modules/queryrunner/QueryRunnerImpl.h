/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "IQueryRunner.h"

namespace queryrunner
{
    class QueryRunnerImpl: public IQueryRunner
    {
        std::string m_osquerySocketPath; 
    public:
        QueryRunnerImpl(const std::string & osquerySocketPath ); 
        ~QueryRunnerImpl() {};
        void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) override;
        virtual std::string id() override; 
        virtual QueryRunnerStatus getResult() override; 
        void requestAbort() override; 
        virtual std::unique_ptr<IQueryRunner> clone() override;
    };
}
