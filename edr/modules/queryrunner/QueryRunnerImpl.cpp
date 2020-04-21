/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "QueryRunnerImpl.h"

namespace queryrunner{
    QueryRunnerImpl::QueryRunnerImpl(const std::string & osquerySocketPath )
    : m_osquerySocketPath{osquerySocketPath}
    {

    } 
    void QueryRunnerImpl::triggerQuery(const std::string& /*correlationid*/, const std::string& /*query*/, std::function<void(std::string id)> /*notifyFinished*/)
    {

    }
    std::string QueryRunnerImpl::id()
    {
        return ""; 
    } 
    QueryRunnerStatus QueryRunnerImpl::getResult()
    {
        QueryRunnerStatus status; 
        return status; 
    } 
    void QueryRunnerImpl::requestAbort(){}; 
    std::unique_ptr<IQueryRunner> QueryRunnerImpl::clone()
    {
        return std::unique_ptr<IQueryRunner>{new QueryRunnerImpl(m_osquerySocketPath)};
    }
}
std::unique_ptr<queryrunner::IQueryRunner> queryrunner::createQueryRunner(std::string osquerySocket)
{
    return std::unique_ptr<queryrunner::IQueryRunner>(new QueryRunnerImpl(osquerySocket)); 
}