// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISignatureVerifierWrapper.h"
#include "ISusRequester.h"

#include "Common/HttpRequests/IHttpRequester.h"

#include <utility>

namespace SulDownloader::SDDS3
{
    class SusRequester : public ISusRequester
    {
    public:
        SusRequester(
            std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient,
            std::unique_ptr<ISignatureVerifierWrapper> verifier);
        SusResponse request(const SUSRequestParameters& parameters) override;

        /**
         * @throws SusResponseVerificationException if the response fails to be verified
         */
        void verifySUSResponse(
            const ISignatureVerifierWrapper& verifier,
            const Common::HttpRequests::Response& response);
        /**
         * @throws SusResponseParseException if the response fails to parse
         */
        void parseSUSResponse(const std::string& response, SusData& data);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_httpClient;
        std::unique_ptr<ISignatureVerifierWrapper> verifier_;
    };
} // namespace SulDownloader::SDDS3