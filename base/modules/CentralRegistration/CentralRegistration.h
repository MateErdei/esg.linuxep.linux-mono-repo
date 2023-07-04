/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <cmcsrouter/ConfigOptions.h>
#include <Common/HttpRequests/IHttpRequester.h>
#include <cmcsrouter/IAdapter.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <map>
#include <memory>

namespace CentralRegistration
{
    class CentralRegistration
    {
    public:
        using MCSHttpClientPtr = std::shared_ptr<MCS::MCSHttpClient>;
        CentralRegistration() = default;
        virtual ~CentralRegistration() = default;

        void registerWithCentral(
                MCS::ConfigOptions& configOptions,
                const std::shared_ptr<Common::HttpRequests::IHttpRequester>& requester,
                const std::shared_ptr<MCS::IAdapter>& agentAdapter);

        static void registerWithCentral(
            MCS::ConfigOptions& configOptions,
            MCS::MCSHttpClient& client,
            const std::shared_ptr<MCS::IAdapter>& agentAdapter);

    protected:
        static void preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        static void preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, MCS::MCSHttpClient& httpClient);
        static std::string processPreregistrationBody(const std::string& preregistrationBody);
        static bool tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient);
        static bool tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient);

        static bool tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, const MCS::MCSHttpClient& httpClient,
               bool (*registrationFunction)(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient));

        MCSHttpClientPtr httpClient_;
    };
}
