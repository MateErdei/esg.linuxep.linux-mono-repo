// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/Proxy.h"
#include "Common/Policy/UpdateSettings.h"
#include "ConnectionSetup.h"

namespace SulDownloader::suldownloaderdata
{
    /**
     * Given the list of sophos urls, update caches and proxies, there are a variety of possible ways to connect to
     * the sophos CDN and Sophos Update Service.
     *
     * ConnectionSelector is responsible to create all the valid ways to connect to the given address while
     * also ordering them in a way to maximize the possibility of connection success.
     */
    class ConnectionSelector
    {
    public:
        /**
         * A method to retrun all the connection setups for acessing sus and the sophos cdn
         * @param the update settings we get from Update scheduler
         * @return two list of connection candidates
         *  first vector is a list of all the ways we can connect to Sus
         *  second vector is a list of all the ways we can connect to a Sophos CDN
         */
        std::pair<std::vector<ConnectionSetup>,std::vector<ConnectionSetup>> getConnectionCandidates(const Common::Policy::UpdateSettings& updateSettings);
    private:
        std::vector<ConnectionSetup> getSDDS3Candidates(std::vector<Common::Policy::Proxy>& proxies,
                                                        std::vector<std::string>& urls,
                                                        const std::string& key,
                                                        std::vector<std::string>& caches);
        bool validateUrl(const std::string& url);
    };
} // namespace SulDownloader::suldownloaderdata
