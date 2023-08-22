// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/Proxy.h"

#include <string>
#include <vector>

namespace SulDownloader::suldownloaderdata
{
    /**
     * Holds enough information to setup a SDDS3 connection
     */
    class ConnectionSetup
    {
    public:
        using proxy_t = Common::Policy::Proxy;

        explicit ConnectionSetup(
            const std::string& updateLocationURL,
            bool isCacheUpdate = false,
            const proxy_t& proxy = proxy_t());

        [[nodiscard]] const proxy_t& getProxy() const;

        [[nodiscard]] const std::string& getUpdateLocationURL() const;

        std::string toString() const;

        bool isCacheUpdate() const;

    private:

        std::string m_updateLocationURL;
        bool m_isUpdateCache;
        proxy_t m_proxy;
    };
} // namespace SulDownloader::suldownloaderdata
