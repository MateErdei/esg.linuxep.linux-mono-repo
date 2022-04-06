/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <cmcsrouter/ConfigOptions.h>
#include <Common/HttpRequests/IHttpRequester.h>
#include <cmcsrouter/IAdapter.h>

#include <map>
#include <memory>

namespace CentralRegistrationImpl
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
        void preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml);
        static std::string processPreregistrationBody(const std::string& preregistrationBody);
        static bool tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy);
        static bool tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy);

        static bool tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token,
               bool (*registrationFunction)(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy));

        static std::shared_ptr<Common::HttpRequests::IHttpRequester> m_requester;
    };
}
