/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Constants.h"

//#include <modules/Common/ObfuscationImpl/SecureCollection.h>
#include <Common/ObfuscationImpl/Obfuscate.h>
#include <Common/ObfuscationImpl/Obscurity.h>

#include <string>

namespace Telemetry::TelemetryConfigImpl
{
    class Proxy
    {
    public:
        Proxy();

        enum Authentication
        {
            none = 0,
            basic = 1,
            digest = 2
        };
        const std::string& getUrl() const;
        void setUrl(const std::string& url);

        unsigned int getPort() const;
        void setPort(unsigned int port);

        Authentication getAuthentication() const;
        void setAuthentication(Authentication authentication);

        const std::string& getUsername() const;
        void setUsername(const std::string& username);

        Common::ObfuscationImpl::SecureString getDeobfuscatedPassword() const;
        const std::string getObfuscatedPassword() const;
        void setPassword(const std::string& obfuscatedPassword);

        bool operator==(const Proxy& rhs) const;
        bool operator!=(const Proxy& rhs) const;

        bool isValidProxy() const;

    private:
        std::string m_url;
        unsigned int m_port;
        Authentication m_authentication;
        std::string m_username;
        std::string m_obfuscatedPassword;
    };
} // namespace Telemetry::TelemetryConfigImpl
