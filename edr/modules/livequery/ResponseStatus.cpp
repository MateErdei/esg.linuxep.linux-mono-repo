/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ResponseStatus.h"
#include <unordered_map>
namespace
{
    std::string fromErrorCode(livequery::ErrorCode errorCode)
    {
        switch (errorCode)
        {
            case livequery::ErrorCode::SUCCESS:
                return "OK";
            case livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING:
                return "Extension exited while running query";
            case livequery::ErrorCode::RESPONSEEXCEEDLIMIT:
                return "Response data exceeded 10MB";
            case livequery::ErrorCode::UNEXPECTEDERROR:
                return "Unexpected error running query";
            case livequery::ErrorCode::OSQUERYERROR:
            default:
                return "Error to be defined";
        }
    }
} // namespace
namespace livequery
{
    ResponseStatus::ResponseStatus(ErrorCode error) : m_errorCode(error), m_errorReason(fromErrorCode(error))
    {
    }

    void ResponseStatus::overrideErrorDescription(const std::string& errorDescription)
    {
        m_errorReason = errorDescription;
    }

    ErrorCode ResponseStatus::errorCode() const
    {
        return m_errorCode;
    }

    int ResponseStatus::errorValue() const
    {
        return static_cast<int>(m_errorCode);
    }

    const std::string& ResponseStatus::errorDescription() const
    {
        return m_errorReason;
    }
        
    std::string ResponseStatus::errorCodeName(ErrorCode errCode)
    {
        switch(errCode)
        {
            case ErrorCode::SUCCESS: 
            return "Success"; 
            case ErrorCode::OSQUERYERROR:
            return "OsqueryError"; 
            case ErrorCode::RESPONSEEXCEEDLIMIT:
            return "ExceedLimit"; 
            case ErrorCode::UNEXPECTEDERROR:
            return "UnexpectedError"; 
            case ErrorCode::EXTENSIONEXITEDWHILERUNNING:
            return "ExtensionExited"; 
        }
        return ""; 
    }

    std::optional<ErrorCode> ResponseStatus::errorCodeFromString(const std::string &  errCodeName)
    {
        static std::unordered_map<std::string, ErrorCode> map2; 
        if (map2.empty())
        {
            for(auto errcode: {ErrorCode::SUCCESS, ErrorCode::OSQUERYERROR, ErrorCode::RESPONSEEXCEEDLIMIT, ErrorCode::UNEXPECTEDERROR, ErrorCode::EXTENSIONEXITEDWHILERUNNING}){
                map2[errorCodeName(errcode)] = errcode;
            }
        }

        auto found = map2.find(errCodeName); 
        if (found == map2.end())
        {
            return std::nullopt; 
        }
        return found->second; 
    }

} // namespace livequery