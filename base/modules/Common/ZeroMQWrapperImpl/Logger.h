// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getZeroMQWrapperImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getZeroMQWrapperImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getZeroMQWrapperImplLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getZeroMQWrapperImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getZeroMQWrapperImplLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getZeroMQWrapperImplLogger(), x) // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getZeroMQWrapperImplLogger(), x) // NOLINT
