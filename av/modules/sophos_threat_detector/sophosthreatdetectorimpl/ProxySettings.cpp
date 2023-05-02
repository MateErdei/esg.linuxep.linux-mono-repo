// Copyright 2023 Sophos All rights reserved.
#include "ProxySettings.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/FileSystem/IFileSystem.h"

#include <thirdparty/nlohmann-json/json.hpp>

namespace
{
    void clearProxy()
    {
        ::unsetenv("https_proxy");
        ::unsetenv("http_proxy");
    }

    std::string getProxy()
    {
        auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        auto* fs = Common::FileSystem::fileSystem();
        try
        {
            auto contents = fs->readFile(path, 2048);
            if (contents.empty())
            {
                return {};
            }
            const auto proxyConfig = nlohmann::json::parse(contents);

            std::string proxy = std::string{"http://"} + proxyConfig["proxy"].get<std::string>();
            return proxy;
        }
        catch (const Common::FileSystem::IFileNotFoundException& ex)
        {
            // mcsrouter deletes the file when using direct connections
        }
        catch (const nlohmann::json::parse_error& exception)
        {
            LOGWARN("Failed to parse " << path << ": " << exception.what());
        }
        return {};
    }
}

namespace sspl::sophosthreatdetectorimpl
{
    void setProxyFromConfigFile()
    {
        auto proxy = getProxy();
        if (proxy.empty())
        {
            LOGINFO("LiveProtection will use direct SXL4 connections");
            clearProxy();
        }
        else
        {
            setenv("https_proxy", proxy.c_str(), 1);
            setenv("http_proxy", proxy.c_str(), 1);
            LOGINFO("LiveProtection will use " << proxy << " for SXL4 connections");
        }
    }
}
