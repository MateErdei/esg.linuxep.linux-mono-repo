// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Logger.h"

// CMake wants "config.h", Bazel wants "pluginimpl/config.h"
#ifdef SPL_BAZEL
#include "pluginimpl/config.h"
#else
#include "config.h"
#endif

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance(LR_PLUGIN_NAME);
    return STATIC_LOGGER;
}
