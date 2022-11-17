// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "SusiScanResultJsonParser.h"

#include "SusiResultUtils.h"

#include <common/StringUtils.h>
#include <pluginimpl/ObfuscationImpl/Base64.h>
#include <thirdparty/nlohmann-json/json.hpp>

namespace threat_scanner
{
    ScanResult parseSusiScanResultJson(const std::string& json) noexcept
    {
        ScanResult results {}; // This is noexcept

        try
        {
            const auto parsedScanResult = nlohmann::json::parse(json);
            for (auto result : parsedScanResult.at("results"))
            {
                const std::string path = Common::ObfuscationImpl::Base64::Decode(result.at("base64path"));

                if (result.contains("detections") && !result["detections"].empty())
                {
                    // Only the first detection should be used per result:
                    // https://sophos.atlassian.net/wiki/spaces/LD/pages/226177287890
                    const auto& detection = result["detections"].at(0);

                    const std::string& name = detection.at("threatName");
                    const std::string& type = detection.at("threatType");

                    // Result is guaranteed to contain SHA256 if there's a detection
                    const std::string& sha256 = result.at("sha256");

                    results.detections.push_back({ path, name, type, sha256 });
                }

                if (result.contains("error"))
                {
                    const std::string escapedPath = common::escapePathForLogging(path);
                    log4cplus::LogLevel logLevel = log4cplus::ERROR_LOG_LEVEL;
                    const std::string errorMsg = susiErrorToReadableError(escapedPath, result["error"], logLevel);
                    results.errors.push_back({ errorMsg, logLevel });
                }
            }
        }
        catch (const std::exception& e)
        {
            results.errors.push_back({ std::string("Failed to parse SUSI response (") + e.what() + "): " + json,
                                       log4cplus::ERROR_LOG_LEVEL });
        }

        // std::vector::push_back has a strong exception guarantee, so 'results' is valid at this point
        return results;
    }
} // namespace threat_scanner