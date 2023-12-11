// Copyright 2022-2023 Sophos Limited. All rights reserved.
#include "ProxyUtils.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <nlohmann/json.hpp>
#include <sstream>

namespace
{
    constexpr auto PROXY_KEY = "proxy";
    constexpr auto CRED_KEY = "credentials";
} // namespace
namespace Common
{
    std::tuple<std::string, std::string> ProxyUtils::getCurrentProxy()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        auto currentProxyFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        std::string proxy;
        std::string credentials;

        if (fileSystem->isFile(currentProxyFilePath))
        {
            try
            {
                std::string fileContents = fileSystem->readFile(currentProxyFilePath);
                if (!fileContents.empty())
                {
                    nlohmann::json j = nlohmann::json::parse(fileContents);
                    if (j.contains(PROXY_KEY))
                    {
                        proxy = j[PROXY_KEY];
                        if (j.contains(CRED_KEY))
                        {
                            credentials = j[CRED_KEY];
                        }
                    }
                }
            }
            catch (const std::exception& exception)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse current proxy file: " << currentProxyFilePath
                             << " with error: " << exception.what();
                throw std::runtime_error(errorMessage.str());
            }
        }
        return { proxy, credentials };
    }

    std::tuple<std::string, std::string> ProxyUtils::deobfuscateProxyCreds(const std::string& creds)
    {
        if (!creds.empty())
        {
            std::vector<std::string> values =
                Common::UtilityImpl::StringUtils::splitString(Common::ObfuscationImpl::SECDeobfuscate(creds), ":");
            if (values.size() == 2)
            {
                std::string username = values[0];
                std::string password = values[1];
                return { username, password };
            }
        }
        return {};
    }

    bool ProxyUtils::updateHttpRequestWithProxyInfo(Common::HttpRequests::RequestConfig& request)
    {
        bool proxySet = false;
        std::string currentProxyFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName();
        try
        {
            auto [address, creds] = Common::ProxyUtils::getCurrentProxy();
            if (!address.empty())
            {
                LOGDEBUG("Proxy address from " << currentProxyFilePath << " file: " << address);
                if (!creds.empty())
                {
                    LOGDEBUG("Proxy in " << currentProxyFilePath << " has credentials");
                    try
                    {
                        auto [username, password] = Common::ProxyUtils::deobfuscateProxyCreds(creds);
                        LOGDEBUG("Deobfuscated credentials from " << currentProxyFilePath);
                        request.proxyUsername = username;
                        request.proxyPassword = password;
                        proxySet = true;
                        request.proxy = address;
                    }
                    catch (const std::exception& ex)
                    {
                        LOGWARN("Failed to deobfuscate proxy credentials: " << ex.what());
                    }
                }
                else
                {
                    proxySet = true;
                    request.proxy = address;
                }
            }
        }
        catch (const std::exception& exception)
        {
            LOGERROR(
                "Exception throw while processing "
                << Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName() << " file");
        }
        return proxySet;
    }
} // namespace Common