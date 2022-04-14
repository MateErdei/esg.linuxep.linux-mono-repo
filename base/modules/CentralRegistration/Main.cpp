/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Main.h"

#include "CentralRegistration.h"
#include "Logger.h"

#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <cmcsrouter/MessageRelay.h>
#include <cmcsrouter/AgentAdapter.h>

#include <Common/UtilityImpl/StringUtils.h>
#include <Common/OSUtilitiesImpl/SystemUtils.h>
#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
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

    MCS::ConfigOptions processCommandLineOptions(const std::vector<std::string>& args, std::shared_ptr<OSUtilities::ISystemUtils> systemUtils)
    {
        std::map<std::string, std::string> configOptions;
        std::string proxyCredentials;
        std::string messageRelaysAsString;
        auto argSize = args.size();

        // Positional args (expect 2 + binary)
        if(argSize < 3)
        {
            LOGERROR("Insufficient positional arguments given. Expecting 2: MCS Token, MCS URL");
            return MCS::ConfigOptions{};
        }
        if(!Common::UtilityImpl::StringUtils::startswith(args[1], "--"))
        {
            configOptions[MCS::MCS_TOKEN] = args[1];
        }
        else
        {
            LOGERROR("Expecting MCS Token, found optional argument: " + args[1]);
            return MCS::ConfigOptions{};
        }
        if (!Common::UtilityImpl::StringUtils::startswith(args[2], "--"))
        {
            configOptions[MCS::MCS_URL] = args[2];
        }
        else
        {
            LOGERROR("Expecting MCS URL, found optional argument: " + args[2]);
            return MCS::ConfigOptions{};
        }

        // Optional args
        for (size_t i=3; i < argSize; i++)
        {
            std::string currentArg(args[i]);

            if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--group"))
            {
                updateConfigOptions(MCS::CENTRAL_GROUP, currentArg, configOptions);
            }
            else if(currentArg == "--customer-token")
            {
                configOptions[MCS::MCS_CUSTOMER_TOKEN] = args[++i];
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--products"))
            {
                updateConfigOptions(MCS::MCS_PRODUCTS, currentArg, configOptions);
            }
            else if(currentArg == "--proxy-credentials")
            {
                proxyCredentials = args[++i];
            }
            else if(currentArg == "--message-relay")
            {
                messageRelaysAsString = args[++i];
            }
            else if(currentArg == "--version")
            {
                configOptions[MCS::VERSION_NUMBER] = args[++i];
            }
        }

        // default set of options, note these options are populated later after registration
        // defaulting here to make is clearer when they are being set.
        configOptions[MCS::MCS_ID] = "";
        configOptions[MCS::MCS_PASSWORD] = "";
        configOptions[MCS::MCS_PRODUCT_VERSION] ="";

        if(proxyCredentials.empty())
        {
            proxyCredentials = systemUtils->getEnvironmentVariable("PROXY_CREDENTIALS");
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

        std::string proxy = systemUtils->getEnvironmentVariable("https_proxy");

        if(proxy.empty())
        {
            proxy = systemUtils->getEnvironmentVariable("http_proxy");
        }

        configOptions[MCS::MCS_PROXY] = proxy;

        configOptions[MCS::MCS_CA_OVERRIDE] = systemUtils->getEnvironmentVariable("MCS_CA");

        std::vector<MCS::MessageRelay> messageRelays = extractMessageRelays(messageRelaysAsString);

        return MCS::ConfigOptions{ messageRelays, configOptions };
    }

    MCS::ConfigOptions registerAndObtainMcsOptions(MCS::ConfigOptions& configOptions)
    {
        CentralRegistration centralRegistration;
        std::shared_ptr<MCS::IAdapter> agentAdapter = std::make_shared<MCS::AgentAdapter>();
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
                std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);

        centralRegistration.registerWithCentral(configOptions, client, agentAdapter);
        // return updated configOptions
        return configOptions;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup loggerSetup;
        // convert args to vector for to remove the need to handle pointers.
        std::vector<std::string> args(argc);
        for(int i=0; i < argc; i++)
        {
            args.emplace_back(argv[i]);
        }

        std::shared_ptr<OSUtilities::ISystemUtils> systemUtils = std::make_shared<OSUtilitiesImpl::SystemUtils>();

        MCS::ConfigOptions configOptions = processCommandLineOptions(args, systemUtils);
        if(configOptions.config.empty())
        {
            return 1;
        }
        configOptions = registerAndObtainMcsOptions(configOptions);
        return 0;
    }

} // namespace CentralRegistrationImpl
