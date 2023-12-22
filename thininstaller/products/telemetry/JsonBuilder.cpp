// Copyright 2023 Sophos Limited. All rights reserved.
#include "JsonBuilder.h"

#include "Logger.h"

#include "Common/OSUtilitiesImpl/SXLMachineID.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <nlohmann/json.hpp>

using namespace thininstaller::telemetry;

JsonBuilder::JsonBuilder(const Common::ConfigFile::ConfigFile& settings,
                         const std::map<std::string, Common::ConfigFile::ConfigFile>& results)
                            : settings_(settings)
                            , results_(results)
{
    tenantId_ = settings_.get("TENANT_ID", "<unknown-tenant-id>");
    machineId_ = Common::OSUtilitiesImpl::SXLMachineID::generateMachineID();
}

static void addCommandLine(nlohmann::json& json, const Common::ConfigFile::ConfigFile& thininstallerArgs)
{
    json["commandLine"] = {
            {"forceInstall", thininstallerArgs.getBoolean("force", false)},
            {"products", thininstallerArgs.contains("products")},
            {"group", thininstallerArgs.contains("group")},
            {"messageRelays", thininstallerArgs.contains("message-relays")},
            {"updateCaches", thininstallerArgs.contains("update-caches")},
    };
}

static void addCommandLine(nlohmann::json& json, const JsonBuilder::map_t& results)
{
    try
    {
        addCommandLine(json, results.at("thininstallerArgs.ini"));
    }
    catch (const std::out_of_range&)
    {
        LOGINFO("No installer arguments telemetry");
    }
}

static void addCommand(nlohmann::json& json, const Common::ConfigFile::ConfigFile& report)
{
    nlohmann::json success;
    success["error_message"] = "";
    success["success"] = true;

    auto summary = report.get("summary", "");

    if (report.getBoolean("productInstalled", false))
    {
        // Everything worked
        json["command"] = {
                {"compatabilityChecksCommand", success},
                {"downloadCommand",            success},
                {"registerCommand",            success},
                {"installCommand",             success}
        };
        return;
    }
    if (!report.getBoolean("productInstalled", true))
    {
        // Download or install failed
        json["command"] = {
                {"compatabilityChecksCommand", success},
                {"registerCommand",            success}
        };
        if (summary == "Failed to install at least one of the packages, see logs for details")
        {
            json["command"]["downloadCommand"] = success;
            json["command"]["installCommand"] = {
                    {"error_message", summary},
                    {"success",       false}
            };
        }
        else
        {
            json["command"]["downloadCommand"] = {
                    {"error_message", summary},
                    {"success",       false}
            };
        }
        return;
    }
    if (!report.getBoolean("registrationWithCentral", true))
    {
        // register failed (not absent)
        assert(!report.getBoolean("registrationWithCentral", false));
        json["command"] = {
                {"compatabilityChecksCommand", success},
                {"registerCommand",            {
                                                       {"error_message", summary},
                                                       {"success", false}
                                               }}
        };
        return;
    }

    json["command"] = {
            {"compatabilityChecksCommand", {
                    {"error_message", summary},
                    {"success", false},
            }
            }
    };
}

static void addCommand(nlohmann::json& json, const JsonBuilder::map_t& results)
{
    try
    {
        addCommand(json, results.at("thininstaller_report.ini"));
    }
    catch (const std::out_of_range&)
    {
        LOGINFO("No installer command telemetry");
    }
}

static void addSystemInfo(nlohmann::json& json, const Common::OSUtilities::IPlatformUtils& platform)
{
    json["systemInfo"] = {
            {"osName", platform.getOsName()},
            {"kernel", platform.getKernelVersion()},
            {"arch", platform.getArchitecture()},
    };
}
std::string JsonBuilder::build(const Common::OSUtilities::IPlatformUtils& platform)
{
    timestamp_ = generateTimeStamp();

    const auto* resultCode = ::getenv("RESULT_CODE");
    bool installSuccess = false;
    if (resultCode)
    {
        std::string result{resultCode};
        if (result == "0")
        {
            installSuccess = true;
        }
    }

    const auto* version = ::getenv("VERSION");
    if (!version)
    {
        version = "<uknown>";
    }

    nlohmann::json json;
    json["timestamp"] = timestamp_;
    json["telemetryType"] = "linuxInstaller";
    json["tenantId"] = tenantId_;
    json["machineId"] = machineId();
    json["linuxInstaller"] = {
        {"installerVersion", version},
        {"installSuccess", installSuccess},
    };

    addCommandLine(json, results_);
    addCommand(json, results_);
    addSystemInfo(json, platform);

    return json.dump();
}

std::string JsonBuilder::generateTimeStamp()
{
    auto now = Common::UtilityImpl::TimeUtils::clock_t::now();
    return Common::UtilityImpl::TimeUtils::MessageTimeStamp(now, Common::UtilityImpl::Granularity::seconds);
}

std::string JsonBuilder::timestamp()
{
    return timestamp_;
}

std::string JsonBuilder::tenantId()
{
    return tenantId_;
}

std::string JsonBuilder::machineId()
{
    return machineId_;
}
