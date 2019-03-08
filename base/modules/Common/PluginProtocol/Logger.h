/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getPluginAPIProtocolLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPIProtocolLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPIProtocolLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginAPIProtocolLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPIProtocolLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPIProtocolLogger(), x)  // NOLINT
