// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ProxyCredentials.h"

#include <string>

namespace Common::Policy
{
    constexpr const char NoProxy[] = "noproxy:";
    constexpr const char EnvironmentProxy[] = "environment:";

    class Proxy
    {
    public:
        explicit Proxy(
            std::string url = "",
            ProxyCredentials credentials = ProxyCredentials());

        bool empty() const { return url_.empty() || url_ == NoProxy; }
        const ProxyCredentials& getCredentials() const { return credentials_; }
        const std::string& getUrl() const { return url_; }

        std::string toStringPostfix() const;

        bool operator==(const Proxy& rhs) const
        {
            return (url_ == rhs.url_ && credentials_ == rhs.credentials_);
        }

        bool operator!=(const Proxy& rhs) const { return !operator==(rhs); }

    private:
        std::string url_;
        ProxyCredentials credentials_;
    };
}
