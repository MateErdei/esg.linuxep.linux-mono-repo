// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "ProxyCredentials.h"

#include <string>

namespace Common::Policy
{
    class Proxy
    {
    public:
        static const std::string NoProxy;
        static const std::string EnvironmentProxy;

        explicit Proxy(
            std::string url = "",
            ProxyCredentials credentials = ProxyCredentials());

        const ProxyCredentials& getCredentials() const;

        const std::string& getUrl() const;
        std::string getProxyUrlAsSulRequires() const;

        bool empty() const;

        bool operator==(const Proxy& rhs) const
        {
            return (m_url == rhs.m_url && m_credentials == rhs.m_credentials);
        }

        bool operator!=(const Proxy& rhs) const { return !operator==(rhs); }

        std::string toStringPostfix() const;

    private:
        std::string m_url;
        ProxyCredentials m_credentials;
    };
}
