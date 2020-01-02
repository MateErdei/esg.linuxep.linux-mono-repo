/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace livequery
{
    enum class ErrorCode : int
    {
        SUCCESS = 0,
        OSQUERYERROR = 1,
        RESPONSEEXCEEDLIMIT = 100,
        EXTENSIONEXITEDWHILERUNNING = 101,
        UNEXPECTEDERROR = 102
    };
    class ResponseStatus
    {
    private:
        ErrorCode m_errorCode;
        std::string m_errorReason;

    public:
        ResponseStatus(ErrorCode error);
        void overrideErrorDescription(const std::string&);

        ErrorCode errorCode() const;
        int errorValue() const;
        const std::string& errorDescription() const;
    };
} // namespace livequery
