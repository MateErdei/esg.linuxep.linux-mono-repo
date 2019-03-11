/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getPluginAPIInterfaceLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPIInterfaceLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPIInterfaceLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginAPIInterfaceLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPIInterfaceLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPIInterfaceLogger(), x)  // NOLINT
