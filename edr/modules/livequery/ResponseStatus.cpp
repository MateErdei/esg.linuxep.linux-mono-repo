/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ResponseStatus.h"
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
} // namespace livequery