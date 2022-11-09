// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServiceCallback.h"
#include "SafeStoreTelemetryConsts.h"

#include "common/ApplicationPaths.h"

#include "safestore/Logger.h"

namespace safestore
{
    void SafeStoreServiceCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGWARN("NotSupported: Received apply new policy: " << policyXml);
    }

    std::string SafeStoreServiceCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }

    Common::PluginApi::StatusInfo SafeStoreServiceCallback::getStatus(const std::string& appid)
    {
        LOGWARN("NotSupported: Received getStatus");
        return Common::PluginApi::StatusInfo{ "", "", appid };
    }

    std::string SafeStoreServiceCallback::getTelemetry()
    {
        LOGDEBUG("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        auto fileSystem = Common::FileSystem::fileSystem();

        bool dormant = fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath());
        unsigned long healthValue = dormant ? 1UL : 0UL;
        std::optional<unsigned long> safeStoreDatabaseSize = getSafeStoreDatabaseSize();

        telemetry.set(telemetrySafeStoreDormantMode, dormant);
        telemetry.set(telemetrySafeStoreHealth, healthValue);

        if (safeStoreDatabaseSize.has_value())
        {
            telemetry.set(telemetrySafeStoreDatabaseSize, safeStoreDatabaseSize.value());
        }

        return telemetry.serialiseAndReset();
    }

    std::optional<unsigned long> SafeStoreServiceCallback::getSafeStoreDatabaseSize()
    {
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            auto files = fileSystem->listFiles(Plugin::getSafeStoreDbDirPath());
            unsigned long size = 0;
            for (auto& file : files)
            {
                size += fileSystem->fileSize(file);
            }
            return size;
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot get size of SafeStore database files");
            return std::nullopt;
        }
    }

    void SafeStoreServiceCallback::queueAction(const std::string& action)
    {
        LOGWARN("NotSupported: Queue action: " << action);
    }
} // namespace safestore