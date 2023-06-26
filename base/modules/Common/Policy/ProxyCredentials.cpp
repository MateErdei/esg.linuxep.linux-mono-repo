// Copyright 2023 Sophos All rights reserved.
#include "ProxyCredentials.h"

#include <utility>

#include "Common/ObfuscationImpl/Obfuscate.h"

using namespace Policy;

ProxyCredentials::ProxyCredentials(
    const std::string& username,
    const std::string& password,
    std::string proxyType) :
    Credentials(username, password),
    m_proxyType(std::move(proxyType))
{
}

Common::ObfuscationImpl::SecureString ProxyCredentials::getDeobfuscatedPassword() const
{
    if (m_proxyType == "2")
    {
        return Common::ObfuscationImpl::SECDeobfuscate(getPassword());
    }
    else
    {
        auto& pwd = getPassword();
        return {pwd.begin(), pwd.end()};
    }
}

const std::string& ProxyCredentials::getProxyType() const
{
    return m_proxyType;
}
