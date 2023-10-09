/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getXmlUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getXmlUtilitiesLogger(), x) // NOLINT
