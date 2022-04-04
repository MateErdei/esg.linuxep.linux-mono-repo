/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Main.h"

#include "CentralRegistration.h"
#include "Logger.h"

#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <cmcsrouter/MessageRelay.h>

#include <Common/UtilityImpl/StringUtils.h>
#include <Logging/ConsoleLoggingSetup.h>
#include <Logging/FileLoggingSetup.h>

namespace CentralRegistrationImpl
{
    void updateConfigOptions(const std::string& key, const std::string& value, std::map<std::string, std::string>& configOptions)
    {
        auto groupArgs = Common::UtilityImpl::StringUtils::splitString(value, "=");
        if(groupArgs.size() == 2)
        {
            configOptions[key] = groupArgs[1];
        }
    }

    std::vector<MCS::MessageRelay> extractMessageRelays(const std::string& messageRelaysAsString)
    {
        std::vector<MCS::MessageRelay> messageRelays;
        std::vector<std::string> splitMessageRelays = Common::UtilityImpl::StringUtils::splitString(messageRelaysAsString, ";");
        for(auto& messageRelayAsString : splitMessageRelays)
        {
            std::vector<std::string> contents = Common::UtilityImpl::StringUtils::splitString(messageRelayAsString, ",");
            if(contents.size() != 3)
            {
                continue;
            }
            std::string priority = contents[1];
            std::string id = contents[2];
            std::vector<std::string> address = Common::UtilityImpl::StringUtils::splitString(contents[0], ":");
            if(address.size() != 2)
            {
                continue;
            }
            std::string hostname = address[0];
            std::string port = address[1];
            messageRelays.emplace_back(MCS::MessageRelay { priority, id, hostname, port });
        }
        return messageRelays;
    }

    MCS::ConfigOptions processCommandLineOptions(int argc, char* argv[] )
    {
        //        parser.add_argument("--deregister",dest="deregister",action="store_true",default=False)
//        parser.add_argument("--reregister",dest="reregister",action="store_true",default=False)
//        parser.add_argument("--messagerelay",dest="messagerelay",action="store",default=None)
//        parser.add_argument("--proxycredentials",dest="proxycredentials",action="store",default=None)
//        parser.add_argument("--central-group",dest="central_group",action="store",default=None)
//        parser.add_argument("--customer-token",dest="customer_token",action="store",default=None)
//        parser.add_argument("--products",dest="selected_products",action="store",default=None)
        std::map<std::string, std::string> configOptions;
        std::string proxyCredentials;
        std::string messageRelaysAsString;

        for (int i=1; i < argc; i++)
        {
            std::string currentArg(argv[i]);

            if((i == 1) && !Common::UtilityImpl::StringUtils::startswith(currentArg, "--"))
            {
                configOptions[MCS::MCS_TOKEN] = currentArg;
            }
            else if ((i==2) && !Common::UtilityImpl::StringUtils::startswith(currentArg, "--"))
            {
                configOptions[MCS::MCS_URL] = currentArg;
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--group"))
            {
                updateConfigOptions(MCS::CENTRAL_GROUP, currentArg, configOptions);
            }
            else if(currentArg == "--customer-token")
            {
                configOptions[MCS::MCS_CUSTOMER_TOKEN] = argv[++i];
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--products"))
            {
                updateConfigOptions(MCS::MCS_PRODUCTS, currentArg, configOptions);
            }
            else if(currentArg == "--proxy-credentials")
            {
                proxyCredentials = argv[++i];
            }
            else if(currentArg == "--message-relay")
            {
                messageRelaysAsString = argv[++i];
            }
        }

        // default set of options, note these options are populated later after registration
        // defaulting here to make is clearer when they are being set.
        configOptions[MCS::MCS_ID] = "";
        configOptions[MCS::MCS_PASSWORD] = "";
        configOptions[MCS::MCS_PRODUCT_VERSION] ="";

        if(proxyCredentials.empty())
        {
            proxyCredentials = std::string(std::getenv("PROXY_CREDENTIALS"));
        }
        if(!proxyCredentials.empty())
        {
            std::vector<std::string> values = Common::UtilityImpl::StringUtils::splitString(proxyCredentials, ":");
            if(values.size() == 2)
            {
                configOptions[MCS::MCS_PROXY_USERNAME] = values[0];
                configOptions[MCS::MCS_PROXY_PASSWORD] = values[1];
            }
        }

        std::string proxy = std::string(std::getenv("https_proxy"));
        if(proxy.empty())
        {
            proxy = std::string(std::getenv("http_proxy"));
        }
        configOptions[MCS::MCS_PROXY] = proxy;

        configOptions[MCS::MCS_CA_OVERRIDE] = std::string(std::getenv("MCS_CA"));

        std::vector<MCS::MessageRelay> messageRelays = extractMessageRelays(messageRelaysAsString);

        return MCS::ConfigOptions{ messageRelays, configOptions };
    }

    MCS::ConfigOptions registerAndObtainMcsOptions(MCS::ConfigOptions& configOptions)
    {
        CentralRegistration centralRegistration;
        centralRegistration.RegisterWithCentral(configOptions);
        // return updated configOptions
        return configOptions;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup loggerSetup;
        MCS::ConfigOptions configOptions = processCommandLineOptions(argc, argv);
        configOptions = registerAndObtainMcsOptions(configOptions);
        return 0;
    }

} // namespace CentralRegistrationImpl
