/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ObfuscationImpl/Obfuscate.h>
#include "Credentials.h"
#include "SulDownloaderException.h"

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

Credentials::Credentials(const std::string& username, const std::string& password)
        : m_username(username)
          , m_password(password)
{
    if (!m_password.empty() && m_username.empty())
    {
        throw SulDownloaderException("Invalid credentials");
    }
}


const std::string& Credentials::getUsername() const
{
    return m_username;
}


const std::string& Credentials::getPassword() const
{
    return m_password;
}



ProxyCredentials::ProxyCredentials(const std::string& username, const std::string& password, const std::string& proxyType)
        : Credentials(username, password)
          , m_proxyType(proxyType)
{

}

Common::ObfuscationImpl::SecureString ProxyCredentials::getDeobfuscatedPassword() const
{
    if(m_proxyType == "2")
    {
        return Common::ObfuscationImpl::SECDeobfuscate(getPassword());
    }
    else
    {
        auto& pwd = getPassword();
        return Common::ObfuscationImpl::SecureString(pwd.begin(), pwd.end());
    }
}

const std::string& ProxyCredentials::getProxyType() const
{
    return m_proxyType;
}

