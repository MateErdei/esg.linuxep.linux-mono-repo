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
}

namespace sspl::sophosthreatdetectorimpl
{
    void setProxyFromConfigFile()
    {
        auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        auto* fs = Common::FileSystem::fileSystem();
        try
        {
            auto contents = fs->readFile(path, 2048);
            if (contents.empty())
            {
                LOGINFO("LiveProtection will use direct SXL4 connections");
                clearProxy();
                return;
            }
            const auto proxyConfig = nlohmann::json::parse(contents);

            std::string proxy = std::string{"http://"} + proxyConfig["proxy"].get<std::string>();

            setenv("https_proxy", proxy.c_str(), 1);
            setenv("http_proxy", proxy.c_str(), 1);
            LOGINFO("LiveProtection will use " << proxy << " for SXL4 connections");
            return;
        }
        catch (const Common::FileSystem::IFileNotFoundException& ex)
        {
            LOGINFO("LiveProtection will use direct SXL4 connections");
        }
        catch (const nlohmann::json::parse_error& exception)
        {
            LOGWARN("Failed to parse " << path << ": " << exception.what());
        }
        clearProxy();
    }
}
