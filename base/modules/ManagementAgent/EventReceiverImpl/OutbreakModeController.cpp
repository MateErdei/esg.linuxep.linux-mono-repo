// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/UtilityImpl/Uuid.h"
#include "FileSystem/IFileNotFoundException.h"
#include "FileSystem/IFileSystemException.h"
#include "ManagementAgent/LoggerImpl/Logger.h"

#include <ctime>
#include <json.hpp>
#include <tuple>

namespace
{
    bool isCountableEvent(
        const std::string& appId,
        const std::string& eventXml)
    {
        std::ignore = appId;

        return Common::UtilityImpl::StringUtils::startswith(eventXml, R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection")");
    }
}

ManagementAgent::EventReceiverImpl::OutbreakModeController::OutbreakModeController()
{
    load();
}


bool ManagementAgent::EventReceiverImpl::OutbreakModeController::processEvent(
    std::string& appId,
    std::string& eventXml)
{
    return processEvent(appId, eventXml, clock_t::now());
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::processEvent(
    std::string& appId,
    std::string& eventXml,
    time_point_t now)
{
    resetCountOnDayChange(now);

    if (!isCountableEvent(appId, eventXml))
    {
        return false;
    }

    if (!outbreakMode_)
    {
        detectionCount_++;
        if (detectionCount_ >= 100)
        {
            LOGWARN("Entering outbreak mode: Further detections will not be reported to Central");
            outbreakMode_ = true;
            appId = "CORE";
            eventXml = generateCoreOutbreakEvent(now);
            save();
            LOGINFO("Replacing detection with outbreak mode report: " << eventXml);
        }
    }
    return false;
}

std::string ManagementAgent::EventReceiverImpl::OutbreakModeController::generateCoreOutbreakEvent(
    ManagementAgent::EventReceiverImpl::OutbreakModeController::time_point_t now)
{
    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(now);
    uuid_ = Common::UtilityImpl::Uuid::CreateVersion5(
        { 0xed, 0xbe, 0x6e, 0x5d,
            0x89, 0x43,
            0x42, 0x6d,
            0xa2, 0x28,
            0x98, 0x92, 0x7d, 0xb0, 0xd8, 0x57
        }, // "edbe6e5d-8943-426d-a228-98927db0d857" Random Namespace
        "OUTBREAK EVENT" // Name
        );
    LOGINFO("Generating Outbreak notification with UUID=" << uuid_);
    return Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"(<event type="sophos.core.outbreak" ts="@@timestamp@@">
<alert id="{{eventCorrelationId}}">
</alert>
</event>)",
        {
            { "@@timestamp@@", timestamp },
            { "{{eventCorrelationId}}", uuid_ },
        });
}

void ManagementAgent::EventReceiverImpl::OutbreakModeController::resetCountOnDayChange(
    ManagementAgent::EventReceiverImpl::OutbreakModeController::time_point_t now)
{
    auto nowTime = clock_t::to_time_t(now);
    struct tm nowTm{};
    localtime_r(&nowTime, &nowTm);
    // We need to compare year, month and day, to avoid aliasing
    // e.g. 99 alters 2023-01-17, and 2 more 2023-02-17, would go into outbreak mode
    // if we only recorded day of month, and didn't get any events in between
    // same issue with year: 2023-01-17 and 2024-01-17, and no events in between
    if (savedDay_ != nowTm.tm_mday || savedMonth_ != nowTm.tm_mon || savedYear_ != nowTm.tm_year)
    {
        // Reset count if the Day Of Month changes
        detectionCount_ = 0;
        savedDay_ = nowTm.tm_mday;
        savedMonth_ = nowTm.tm_mon;
        savedYear_ = nowTm.tm_year;
    }
}

void ManagementAgent::EventReceiverImpl::OutbreakModeController::save()
{
    auto* filesystem = Common::FileSystem::fileSystem();
    auto& paths = Common::ApplicationConfiguration::applicationPathManager();
    auto path = paths.getOutbreakModeStatusFilePath();
    nlohmann::json j;
    j["outbreakMode"] = outbreakMode_;
    if (!uuid_.empty())
    {
        j["uuid"] = uuid_;
    }
    auto contents = j.dump();
    try
    {
        filesystem->writeFileAtomically(path, contents, paths.getTempPath(), 0600);
    }
    catch (const Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to write outbreak status to file: " << ex.what());
    }
}

void ManagementAgent::EventReceiverImpl::OutbreakModeController::load()
{
    auto* filesystem = Common::FileSystem::fileSystem();
    auto& paths = Common::ApplicationConfiguration::applicationPathManager();
    auto path = paths.getOutbreakModeStatusFilePath();

    try
    {
        auto contents = filesystem->readFile(path, 1024); // catch exceptions and default
        if (!contents.empty())
        {
            // Ignore empty files without error
            nlohmann::json j = nlohmann::json::parse(contents);
            outbreakMode_ = j.at("outbreakMode").get<bool>();
            if (j.contains("uuid"))
            {
                uuid_ = j.at("uuid").get<std::string>();
            }
        }
    }
    catch (const Common::FileSystem::IFileNotFoundException& ex)
    {
        // Using defaults - no file found
        LOGDEBUG("Unable to read outbreak status file as it doesn't exist");
    }
    catch (const Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Unable to read outbreak status file: " << ex.what());
    }
    catch (const nlohmann::json::parse_error& ex)
    {
        LOGERROR("Failed to parse outbreak status file: " << ex.what());
    }
    catch (const nlohmann::json::out_of_range& ex)
    {
        LOGERROR("Missing key in outbreak status file: " << ex.what());
    }
    catch (const nlohmann::json::type_error& ex)
    {
        LOGERROR("Invalid type for value in outbreak status file: " << ex.what());
    }

    if (outbreakMode_)
    {
        LOGWARN("In outbreak mode: Detections will not be reported to Central");
    }
    else
    {
        LOGDEBUG("Not in outbreak mode");
    }
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::outbreakMode() const
{
    return outbreakMode_;
}