/******************************************************************************************************

Copyright Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include <string>

namespace livequery{
    enum class ErrorCode{SUCCESS, ERROR};
    struct ResponseStatus {
        ErrorCode m_errorCode;
        std::string m_errorReason;
        // todo extract this information from osquery status
    };
}
