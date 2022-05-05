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
        CentralRegistration() = default;
        ~CentralRegistration() = default;

        void registerWithCentral(
                MCS::ConfigOptions& configOptions,
                std::shared_ptr<Common::HttpRequests::IHttpRequester> requester,
                std::shared_ptr<MCS::IAdapter> agentAdapter);

    private:
        void preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        static std::string processPreregistrationBody(const std::string& preregistrationBody);
        static bool tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient);
        static bool tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient);

        static bool tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, const MCS::MCSHttpClient& httpClient,
               bool (*registrationFunction)(MCS::ConfigOptions& configOptions, const std::string& statusXml, const std::string& proxy, MCS::MCSHttpClient httpClient));
    };
}
