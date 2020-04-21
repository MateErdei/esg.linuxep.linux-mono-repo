/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Process/IProcess.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include "Logger.h"
#include "QueryRunnerImpl.h"

namespace queryrunner{
    QueryRunnerImpl::QueryRunnerImpl(const std::string &osquerySocketPath, const std::string &executablePath)
    : m_osquerySocketPath{osquerySocketPath}, m_executablePath(executablePath)
    {

    } 
    void QueryRunnerImpl::triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> /*notifyFinished*/)
    {
        Common::Process::IProcessPtr processMonitorPtr = Common::Process::createProcess();
        std::vector<std::string> arguments ={correlationid,query,m_osquerySocketPath};
        processMonitorPtr->exec(m_executablePath, arguments, {});

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
        return std::unique_ptr<IQueryRunner>{new QueryRunnerImpl(m_osquerySocketPath, m_executablePath)};
    }
}
std::unique_ptr<queryrunner::IQueryRunner> queryrunner::createQueryRunner(std::string osquerySocket, std::string executablePath)
{
    return std::unique_ptr<queryrunner::IQueryRunner>(new QueryRunnerImpl(osquerySocket, executablePath));
}