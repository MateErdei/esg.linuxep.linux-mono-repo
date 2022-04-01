/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <map>

namespace CentralRegistrationImpl
{
    class CentralRegistration
    {
    public:
        CentralRegistration() = default;
        ~CentralRegistration() = default;

        void RegisterWithCentral(std::map<std::string, std::string>& configOptions);

    private:
        void preregistration(std::map<std::string, std::string>& configOptions, const std::string& statusXml, const std::string& mcsCert);
        std::string processPreregistrationBody(const std::string& preregistrationBody);

    };

}