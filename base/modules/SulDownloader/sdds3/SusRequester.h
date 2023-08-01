// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISusRequester.h"

#include "Common/HttpRequests/IHttpRequester.h"

#include <utility>

namespace SulDownloader::SDDS3
{
    class SusRequester : public ISusRequester
    {
    public:
        explicit SusRequester(std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient);
        SusResponse request(const SUSRequestParameters& parameters) override;
        void parseSUSResponse(const std::string& response, SusData& data);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_httpClient;
    };
} // namespace SulDownloader::SDDS3