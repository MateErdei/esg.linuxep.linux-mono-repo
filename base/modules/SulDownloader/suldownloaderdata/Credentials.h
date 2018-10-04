/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
#include <Common/ObfuscationImpl/SecureCollection.h>

namespace SulDownloader
{

    namespace suldownloaderdata
    {
        class Credentials
        {

        public:
            explicit Credentials(const std::string& username = "", const std::string& password = "");

            const std::string& getUsername() const;

            const std::string& getPassword() const;

            bool operator==(const Credentials& rhs) const
            {
                return (m_username == rhs.m_username && m_password == rhs.m_password);
            }

            bool operator!=(const Credentials& rhs) const
            {
                return !operator==(rhs);
            }

        private:
            std::string m_username;
            std::string m_password;

        };

        class ProxyCredentials : public Credentials
        {
        public:
            explicit ProxyCredentials(const std::string& username = "", const std::string& password = "", const std::string& m_proxyType = "");

            Common::ObfuscationImpl::SecureString getDeobfuscatedPassword() const;
            const std::string& getProxyType() const;

            bool operator==(const ProxyCredentials& rhs) const
            {
                return ( Credentials::operator==(rhs) && m_proxyType == rhs.m_proxyType);
            }

            bool operator!=(const ProxyCredentials& rhs) const
            {
                return !operator==(rhs);
            }

        private:

            std::string m_proxyType;
        };
    }

}

