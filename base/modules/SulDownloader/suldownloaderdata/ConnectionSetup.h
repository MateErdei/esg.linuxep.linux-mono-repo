// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/Credentials.h"
#include "Common/Policy/Proxy.h"

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        /**
         * Holds enough information to setup a SUL connection required by the WarehouseRepository.
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
            void setProxy(const proxy_t& proxy);

            [[nodiscard]] const std::string& getUpdateLocationURL() const;
            void setUpdateLocationURL(const std::string& updateLocationURL);

            std::string toString() const;

            bool isCacheUpdate() const;

        private:
            std::string m_updateLocationURL;
            bool m_isUpdateCache;
            proxy_t m_proxy;
        };

    } // namespace suldownloaderdata
} // namespace SulDownloader
