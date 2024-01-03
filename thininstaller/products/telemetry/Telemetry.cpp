// Copyright 2023 Sophos Limited. All rights reserved.
#include "Telemetry.h"

#include "JsonBuilder.h"
#include "Logger.h"
#include "TelemetryEnabled.h"

#include "Common/ConfigFile/ConfigFile.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <cassert>
#include <sstream>

thininstaller::telemetry::Telemetry::Telemetry(std::vector<std::string> args, Common::HttpRequests::IHttpRequesterPtr httpRequester)
    : args_(std::move(args))
    , requester_(std::move(httpRequester))
{
}

int thininstaller::telemetry::Telemetry::run(Common::FileSystem::IFileSystem* fs, const Common::OSUtilities::IPlatformUtils& platform)
{
    using Common::ConfigFile::ConfigFile;

    // Load configuration file
    if (args_.empty())
    {
        LOGERROR("No configuration specified");
        return 1;
    }
    assert(fs);
    ConfigFile config = ConfigFile(fs, args_[0]);

    // Determine if we should send telemetry
    auto url = config.get("URL", "");
    if (!isTelemetryEnabled(url))
    {
        LOGINFO("Telemetry disabled");
        return 0;
    }

    std::ostringstream argsStr;
    for (const auto& arg : args_)
    {
        argsStr << arg << ", ";
    }
    LOGDEBUG("Telemetry for: " << argsStr.str());

    // Load result files
    std::map<std::string, ConfigFile> results;
    for (unsigned pos=1; pos<args_.size(); pos++)
    {
        std::string path = args_[pos];
        std::string basename = Common::FileSystem::basename(path);
        results.emplace(basename, ConfigFile{fs, path});

        auto contents = fs->readFile(path);
        LOGDEBUG(path << " == " << contents);
    }

    // Craft JSON
    thininstaller::telemetry::JsonBuilder builder{config, results};
    auto json = builder.build(platform);
    if (json.empty())
    {
        return 0;
    }

    LOGDEBUG("Sending telemetry: " << json);

    // Craft URL
    std::stringstream telemetryUrl;
    telemetryUrl << "https://linuxinstaller-telemetry.sophosupd.com/uploads/"
                 << builder.tenantId()
                 << "_" << builder.machineId()
                 << "_" << builder.timestamp()
                 << ".json";

    // Send Telemetry
    url_ = telemetryUrl.str();
    json_ = json;
    sendTelemetry(url_, json_);

    return 0;
}

void thininstaller::telemetry::Telemetry::sendTelemetry(const std::string& url, const std::string& telemetryJson)
{
    LOGDEBUG("Sending telemetry to " << url);

    Common::HttpRequests::Headers headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("Expect", "100-continue");

    Common::HttpRequests::RequestConfig requestConfig;
    requestConfig.url = url;
    requestConfig.timeout = 300;
    requestConfig.data = telemetryJson;
    requestConfig.headers = std::move(headers);

    // TODO Proxy options

    // Do request
    assert(requester_);
    requester_->put(requestConfig);
    LOGDEBUG("Sent telemetry");
}

std::string thininstaller::telemetry::Telemetry::json()
{
    return json_;
}

std::string thininstaller::telemetry::Telemetry::url()
{
    return url_;
}
