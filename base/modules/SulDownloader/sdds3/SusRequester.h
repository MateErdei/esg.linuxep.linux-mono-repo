/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once
#include "SusRequestParameters.h"

#include <Common/HttpRequests/IHttpRequester.h>

#include <set>
#include <string>
#include <utility>
namespace SulDownloader::SDDS3
{
    struct SusData
    {
        std::set<std::string> suites;
        std::set<std::string> releaseGroups;
    };

    struct SusResponse
    {
        SusData data;
        bool success = false;
        std::string error;
    };

    class SusRequester
    {
    public:
        explicit SusRequester(std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient);
        SusResponse request(const SUSRequestParameters& parameters);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_httpClient;
    };
} // namespace SulDownloader::SDDS3