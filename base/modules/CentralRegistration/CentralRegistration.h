/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <cmcsrouter/ConfigOptions.h>

#include <map>

namespace CentralRegistrationImpl
{
    class CentralRegistration
    {
    public:
        CentralRegistration() = default;
        ~CentralRegistration() = default;

        void RegisterWithCentral(MCS::ConfigOptions& configOptions);

    private:
        void preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml);
        static std::string processPreregistrationBody(const std::string& preregistrationBody);
        static bool tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy);
        static bool tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy);

        static void tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token,
                       bool (*func)(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy));

    };

}