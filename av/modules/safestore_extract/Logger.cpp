// Copyright 2022-2024 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getSafeStoreExtractorLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("safestore_extractor");
    return STATIC_LOGGER;
}