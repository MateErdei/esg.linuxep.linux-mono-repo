// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServiceCallback.h"
#include "SafeStoreTelemetryConsts.h"

#include "common/ApplicationPaths.h"
#include "Common/FileSystem/IFileSystemException.h"

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

        telemetry.increment(telemetrySafeStoreQuarantineSuccess, 0ul);
        telemetry.increment(telemetrySafeStoreQuarantineFailure, 0ul);
        telemetry.increment(telemetrySafeStoreUnlinkFailure, 0ul);

        if (safeStoreDatabaseSize.has_value())
        {
            telemetry.set(telemetrySafeStoreDatabaseSize, safeStoreDatabaseSize.value());
        }

        return telemetry.serialiseAndReset();
    }

    std::optional<unsigned long> SafeStoreServiceCallback::getSafeStoreDatabaseSize() const
    {
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            auto files = fileSystem->listFiles(Plugin::getSafeStoreDbDirPath());
            unsigned long size = 0;
            for (auto& file : files)
            {
                auto tmpSize = fileSystem->fileSize(file);

                if (tmpSize > 0)
                {
                    size += tmpSize;
                }
                else
                {
                    LOGWARN("Failed to get size of " << file << " while computing SafeStore database size for telemetry");
                }
            }
            return size;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
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