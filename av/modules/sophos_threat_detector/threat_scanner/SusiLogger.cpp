/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiLogger.h"

#include "Logger.h"

#include <string>

namespace
{
    const std::string WHITESPACE = " \n\r\t\f\v";

    std::string rtrim(const std::string& s)
    {
        size_t end = s.find_last_not_of(WHITESPACE);
        return (end == std::string::npos) ? "" : s.substr(0, end + 1);
    }
}

void threat_scanner::susiLogCallback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    m = rtrim(m);

    if (!m.empty())
    {
        switch (level)
        {
            case SUSI_LOG_LEVEL_DETAIL:
                LOG_SUSI_DEBUG(m); // Not putting debug in normal log
                break;
            case SUSI_LOG_LEVEL_INFO:
                // SUSI is too verbose at info-level, downgrade to support-level
                LOG_SUSI_SUPPORT(m); // Not putting info in normal log
                break;
            case SUSI_LOG_LEVEL_WARNING:
                LOGWARN(m);
                LOG_SUSI_WARN(m);
                break;
            case SUSI_LOG_LEVEL_ERROR:
                LOGERROR(m);
                LOG_SUSI_ERROR(m);
                break;
            default:
                LOGERROR(level << ": " << m);
                LOG_SUSI_ERROR(level << ": " << m);
                break;
        }
    }
}
void threat_scanner::fallbackSusiLogCallback(void*, SusiLogLevel level, const char* message)
{

    std::string m(message);
    if (!m.empty())
    {
        switch (level)
        {
            case SUSI_LOG_LEVEL_DETAIL:
                std::cout << "DEBUG: " << m;
                break;
            case SUSI_LOG_LEVEL_INFO:
                std::cout << "INFO: " << m;
                break;
            case SUSI_LOG_LEVEL_WARNING:
                std::cout << "WARNING: " << m;
                break;
            case SUSI_LOG_LEVEL_ERROR:
                std::cerr << "ERROR: " << m;
                break;
            default:
                std::cerr << level << ": " << m;
                break;
        }
    }
}
