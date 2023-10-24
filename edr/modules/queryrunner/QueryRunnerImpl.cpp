// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "QueryRunnerImpl.h"

#include "EdrCommon/ApplicationPaths.h"

#include "Common/Process/IProcess.h"
#include "Common/TelemetryHelperImpl/TelemetryJsonToMap.h"

#define SECONDS_UNTIL_SIGKILL 3

namespace
{
    const char * QueryName = "name"; 
    const char * ErrorCode = "errorcode"; 
    const char * Duration = "duration"; 
    const char * RowCount = "rowcount";
    const uint QUERY_TIMEOUT_MINS = 10;
}

namespace queryrunner{
    QueryRunnerImpl::QueryRunnerImpl(const std::string &osquerySocketPath, const std::string &executablePath)
    : m_osquerySocketPath{osquerySocketPath}, m_executablePath(executablePath), m_runnerStatus({})
    {
        

    } 
    void QueryRunnerImpl::triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished)
    {
        m_correlationId = correlationid; 
        m_fut = std::async( std::launch::async, [this, correlationid, query, notifyFinished]()
        {
            try
            {
                this->m_process = Common::Process::createProcess(); 
                LOGINFO("Trigger livequery at: " << this->m_executablePath << " for query : " << correlationid);
                std::vector<std::string> arguments = {correlationid, query, this->m_osquerySocketPath};            
                this->m_process->exec(this->m_executablePath, arguments, {});
                Common::Process::ProcessStatus processStatus = this->m_process->wait(std::chrono::minutes (QUERY_TIMEOUT_MINS), 1);
                if (processStatus != Common::Process::ProcessStatus::FINISHED)
                {
                    this->m_process->kill(SECONDS_UNTIL_SIGKILL);
                    LOGWARN("Live query process was stopped due to a timeout after "
                             << QUERY_TIMEOUT_MINS << " mins, correlation ID: " << correlationid);
                }
                auto output = this->m_process->output();
                auto code = this->m_process->exitCode(); 
                this->setStatus(code, output);
            }
            catch (std::exception & ex)
            {
                LOGERROR("Error while running LiveQuery: " << ex.what()); 
                this->setStatus(2, ""); 
            }
            notifyFinished(correlationid); 
        }); 
    }
    std::string QueryRunnerImpl::id()
    {
        return m_correlationId; 
    } 
    QueryRunnerStatus QueryRunnerImpl::getResult()
    {
        std::lock_guard<std::mutex> l{m_mutex}; 
        return m_runnerStatus; 
    } 
    void QueryRunnerImpl::setStatusFromExitResult( QueryRunnerStatus& queryrunnerStatus, int exitCode, const std::string & output)
    {
        queryrunnerStatus.name = ""; 
        queryrunnerStatus.queryDuration = 0; 
        queryrunnerStatus.rowCount = 0;        
        while(true)
        {
            if (exitCode != 0)
            {
                LOGINFO("Execution of livequery failed with error code: " << exitCode);
                break; 
            }
            auto pos = std::find(output.begin(), output.end(), '{'); 
            if (pos == output.end())
            {
                LOGINFO("LiveQuery did not produce a json value" );
                break; 
            }
            std::string responseJson(pos, output.end());
            std::unordered_map<std::string, Common::Telemetry::TelemetryValue> responseMap;
            try
            {
                responseMap = Common::Telemetry::flatJsonToMap(responseJson);
            }
            catch (std::runtime_error& ex)
            {
                LOGWARN("Received invalid json response from sophos livequery:  " << output);
                LOGSUPPORT( ex.what());
                break; 
            }
            auto queryName = responseMap.find(QueryName); 
            auto errorCode = responseMap.find(ErrorCode); 
            auto duration = responseMap.find(Duration); 
            auto rowCount = responseMap.find(RowCount); 
            if (errorCode == responseMap.end())
            {
                LOGINFO("LiveQuery json value did not contain: " << ErrorCode );
                break; 
            }
            auto errC = livequery::ResponseStatus::errorCodeFromString(errorCode->second.getString());
            if ( !errC)
            {
                LOGINFO("LiveQuery presented invalid value for ErrorCode: " << errorCode->second.getString() );
                break; 
            } 
            queryrunnerStatus.errorCode = errC.value(); 
            if (queryName != responseMap.end())
            {
                if (queryName->second.getType() == Common::Telemetry::TelemetryValue::Type::string_type)
                {
                    queryrunnerStatus.name = queryName->second.getString(); 
                }                
            }
            if (duration != responseMap.end())
            {
                             
                if (duration->second.getType() == Common::Telemetry::TelemetryValue::Type::unsigned_integer_type)
                {
                    queryrunnerStatus.queryDuration = duration->second.getUnsignedInteger(); 
                }
            }
            if (rowCount != responseMap.end())
            {
                if (rowCount->second.getType() == Common::Telemetry::TelemetryValue::Type::unsigned_integer_type)
                {
                    queryrunnerStatus.rowCount = rowCount->second.getUnsignedInteger(); 
                }
            }
            return; 
        }
        LOGINFO("Extra information, output of livequery: " << output);
        queryrunnerStatus.errorCode = livequery::ErrorCode::UNEXPECTEDERROR;
    }

    void  QueryRunnerImpl::setStatus(int exitCode, const std::string& output)
    {
        std::lock_guard<std::mutex> l{m_mutex}; 
        setStatusFromExitResult(m_runnerStatus, exitCode, output); 
        LOGDEBUG("Query finished: " << m_runnerStatus.name << ", exit code: "
            << livequery::ResponseStatus::errorCodeName(m_runnerStatus.errorCode) 
            << ". Duration: " << m_runnerStatus.queryDuration << ". Rows: " << m_runnerStatus.rowCount); 
    }

    void QueryRunnerImpl::requestAbort()
    {
        m_process->kill(SECONDS_UNTIL_SIGKILL);
    };

    std::unique_ptr<IQueryRunner> QueryRunnerImpl::clone()
    {
        return std::unique_ptr<IQueryRunner>{new QueryRunnerImpl(m_osquerySocketPath, m_executablePath)};
    }
}

std::unique_ptr<queryrunner::IQueryRunner> queryrunner::createQueryRunner(std::string osquerySocket, std::string executablePath)
{
    return std::unique_ptr<queryrunner::IQueryRunner>(new QueryRunnerImpl(osquerySocket, executablePath));
}