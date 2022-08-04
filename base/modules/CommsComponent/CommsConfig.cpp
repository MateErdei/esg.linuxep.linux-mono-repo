/******************************************************************************************************

Copyright 2020 - 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommsConfig.h"

#include "Logger.h"

#include "ProxyUtils/ProxyUtils.h"

#include <Common/ObfuscationImpl/Obfuscate.h>

#include <fstream>
#include <string>

namespace
{
    constexpr auto GL_PROXY = "proxy";
    constexpr auto GL_CRED = "credentials";
} // namespace

namespace CommsComponent
{
    void CommsConfig::addProxyInfoToConfig()
    {
        try
        {
            auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
            if (!proxy.empty())
            {
                setProxy(proxy);
                if (!credentials.empty())
                {
                    auto deobfuscated = Common::ObfuscationImpl::SECDeobfuscate(credentials);
                    setDeobfuscatedCredential(deobfuscated);
                }
            }
        }
        catch (const std::exception& exception)
        {
            LOGERROR("Failed to add proxy info to config: " << exception.what());
        }
    }
    std::map<std::string, std::vector<std::string>> CommsConfig::getKeyList() const
    {
        return m_key_composed_values_config;
    }

    void CommsConfig::addKeyValueToList(std::pair<std::string, std::vector<std::string>> keyList)
    {
        m_key_composed_values_config.insert(keyList);
    }

    void CommsConfig::setProxy(const std::string& proxy)
    {
        LOGDEBUG("Storing proxy info");
        addKeyValueToList(std::pair<std::string, std::vector<std::string>>(GL_PROXY, { proxy }));
    }
    void CommsConfig::setDeobfuscatedCredential(const std::string& credential)
    {
        LOGDEBUG("Storing credential info");
        addKeyValueToList(std::pair<std::string, std::vector<std::string>>(GL_CRED, { credential }));
    }
    std::string CommsConfig::getProxy() const
    {
        return getSimpleConfigValue(GL_PROXY);
    }
    std::string CommsConfig::getDeobfuscatedCredential() const
    {
        return getSimpleConfigValue(GL_CRED);
    }
    std::string CommsConfig::getSimpleConfigValue(const std::string& key) const
    {
        auto found = m_key_composed_values_config.find(key);
        if (found == m_key_composed_values_config.end())
        {
            return std::string{};
        }
        if (found->second.empty())
        {
            return std::string{};
        }
        if (found->second.size() > 1)
        {
            throw std::logic_error("The key does not refer to a simple string value");
        }
        return found->second.at(0);
    }

} // namespace CommsComponent