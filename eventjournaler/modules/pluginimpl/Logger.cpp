// Copyright 2021 Sophos Limited. All rights reserved.
#include "Logger.h"
#ifdef SPL_BAZEL
#include "pluginimpl/config.h"
#else
#include "config.h"
#endif
#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance(PLUGIN_NAME);
    return STATIC_LOGGER;
}
