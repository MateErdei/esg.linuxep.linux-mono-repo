// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/HttpRequests/IHttpRequester.h"
#include "cmcsrouter/ConfigOptions.h"
#include "cmcsrouter/IAdapter.h"
#include "cmcsrouter/MCSHttpClient.h"

#include <map>
#include <memory>

namespace CentralRegistration
{
    class CentralRegistration
    {
    public:
        CentralRegistration() = default;
        virtual ~CentralRegistration() = default;

        static void registerWithCentral(
            MCS::ConfigOptions& configOptions,
            const std::shared_ptr<MCS::MCSHttpClient>& client,
            const std::shared_ptr<MCS::IAdapter>& agentAdapter);

    private:
        static void preregistration(
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            const std::shared_ptr<MCS::MCSHttpClient>& httpClient);
        static std::string processPreregistrationBody(const std::string& preregistrationBody);
        static bool tryPreregistration(
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            const std::string& proxy,
            const std::shared_ptr<MCS::MCSHttpClient>& httpClient);
        static bool tryRegistration(
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            const std::string& proxy,
            const std::shared_ptr<MCS::MCSHttpClient>& httpClient);

        static bool tryRegistrationWithProxies(
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            const std::shared_ptr<MCS::MCSHttpClient>& httpClient,
            bool (*registrationFunction)(
                MCS::ConfigOptions& configOptions,
                const std::string& statusXml,
                const std::string& proxy,
                const std::shared_ptr<MCS::MCSHttpClient>& httpClient));

        static void writeToIni(bool usedProxy, bool usedMessageRelay, const std::string& proxyOrMessageRelayURL = "");
    };
} // namespace CentralRegistration
