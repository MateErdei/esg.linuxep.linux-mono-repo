// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include "EventUtils.h"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/UtilityImpl/Uuid.h"
#include "FileSystem/IFileNotFoundException.h"
#include "ManagementAgent/LoggerImpl/Logger.h"
#include "XmlUtilities/AttributesMap.h"

#include <ctime>
#include <json.hpp>

ManagementAgent::EventReceiverImpl::OutbreakModeController::OutbreakModeController()
{
    load();
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::processEvent(const Event& event)
{
    return processEvent(event, clock_t::now());
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::processEvent(
    const Event& event,
    const time_point_t now)
{
    // We always reset the day count, even for non-countable/non-blockable events
    resetCountOnDayChange(now);

    if (!event.isBlockableEvent()) // Current assumed to be a superclass of countable
    {
        return false;
    }

    if (outbreakMode_)
    {
        // assert is blockable here
        return true;
    }

    if (!event.isCountableEvent())
    {
        // Assert not in outbreak mode here
        return false;
    }

    // Assert countable event here
    detectionCount_++;
    if (detectionCount_ >= 100)
    {
        LOGWARN("Entering outbreak mode: Further detections will not be reported to Central");
        outbreakMode_ = true;
        // Sent before the 100th event, but shouldn't cause huge problems
        auto outbreakXml = generateCoreOutbreakEvent(now);
        LOGDEBUG("Sending outbreak mode report: " << outbreakXml);
        ManagementAgent::EventReceiverImpl::sendEvent({"CORE", outbreakXml});
        save();
    }
    return false; // Still send the 100th event
}


std::string ManagementAgent::EventReceiverImpl::OutbreakModeController::generateUUID()
{
    return Common::UtilityImpl::Uuid::CreateVersion5(
        { 0xed, 0xbe, 0x6e, 0x5d,
          0x89, 0x43,
          0x42, 0x6d,
          0xa2, 0x28,
          0x98, 0x92, 0x7d, 0xb0, 0xd8, 0x57
        }, // "edbe6e5d-8943-426d-a228-98927db0d857" Random Namespace
        "OUTBREAK EVENT" // Name
    );
}


std::string ManagementAgent::EventReceiverImpl::OutbreakModeController::generateCoreOutbreakEvent(
    ManagementAgent::EventReceiverImpl::OutbreakModeController::time_point_t now)
{
    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(now);
    uuid_ = generateUUID();
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
            else
            {
                uuid_ = generateUUID();
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

void ManagementAgent::EventReceiverImpl::OutbreakModeController::processAction(const std::string& actionXml)
{
    if (!outbreakMode_)
    {
        // no point parsing XML if we aren't in outbreak mode
        return;
    }
    auto xml = Common::XmlUtilities::parseXml(actionXml);
    auto action = xml.lookup("action");
    if (action.value("type", "") == "sophos.core.threat.sav.clear")
    {
        auto items = xml.lookupMultiple("action/item");
        for (const auto& item : items)
        {
            if (item.value("id") == uuid_)
            {
                leaveOutbreakMode();
                return; // no point searching further
            }
        }
    }
}

void ManagementAgent::EventReceiverImpl::OutbreakModeController::leaveOutbreakMode()
{
    LOGINFO("Leaving outbreak mode");
    outbreakMode_ = false;
    uuid_ = "";
    save();
}
