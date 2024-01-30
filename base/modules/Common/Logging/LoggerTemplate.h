// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getNameOfMyLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getNameOfMyLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getNameOfMyLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getNameOfMyLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getNameOfMyLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getNameOfMyLogger(), x)

/*
 * 1. Copy the lines above and rename it Logger.h
 * 2. Replace NameOfMy to the name of the module that you want the log for
 * 3. Create a Logger.cpp file with the following content:*/
///******************************************************************************************************
//
// Copyright 2018-2019, Sophos Limited.  All rights reserved.
//
//******************************************************************************************************/
// #include "Logger.h"
// #include "Common/Logging/LoggerConfig.h"
//
// log4cplus::Logger& getNameOfMyLoggerLogger()
//{
//    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("nameofmy");
//    return STATIC_LOGGER;
//}
