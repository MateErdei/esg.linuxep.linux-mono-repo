// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Main.h"

#include "CentralRegistration.h"
#include "Logger.h"
#include "MessageRelayExtractor.h"
#include "MessageRelaySorter.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/OSUtilitiesImpl/SystemUtils.h"
#include "Common/Obfuscation/ICipherException.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "cmcsrouter/AgentAdapter.h"
#include "cmcsrouter/Config.h"
#include "cmcsrouter/ConfigOptions.h"
#include "cmcsrouter/MessageRelay.h"

namespace CentralRegistration
{
    void updateConfigOptions(
        const std::string& key,
        const std::string& value,
        std::map<std::string, std::string>& configOptions)
    {
        auto groupArgs = Common::UtilityImpl::StringUtils::splitString(value, "=");
        if (groupArgs.size() == 2)
        {
            configOptions[key] = groupArgs[1];
        }
    }

    bool allowedToUseMcsCertOverride(std::shared_ptr<OSUtilities::ISystemUtils> systemUtils)
    {
        return systemUtils->getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA") == "--allow-override-mcs-ca" ||
               Common::FileSystem::fileSystem()->exists(
                   Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag());
    }

    MCS::ConfigOptions processCommandLineOptions(
        const std::vector<std::string>& args,
        const std::shared_ptr<OSUtilities::ISystemUtils>& systemUtils)
    {
        std::map<std::string, std::string> configOptions;
        std::string proxyCredentials;
        std::string obscuredProxyCredentials;
        std::string messageRelaysAsString;
        auto argSize = args.size();

        // Positional args (expect 2 + binary)
        if (argSize < 2)
        {
            LOGERROR("Insufficient positional arguments given. Expecting 2: MCS Token, MCS URL");
            return MCS::ConfigOptions{};
        }
        if (!Common::UtilityImpl::StringUtils::startswith(args[0], "--"))
        {
            configOptions[MCS::MCS_TOKEN] = args[0];
        }
        else
        {
            LOGERROR("Expecting MCS Token, found optional argument: " + args[0]);
            return MCS::ConfigOptions{};
        }
        if (!Common::UtilityImpl::StringUtils::startswith(args[1], "--"))
        {
            configOptions[MCS::MCS_URL] = args[1];
        }
        else
        {
            LOGERROR("Expecting MCS URL, found optional argument: " + args[1]);
            return MCS::ConfigOptions{};
        }

        // Optional args
        for (size_t i = 2; i < argSize; ++i)
        {
            const std::string& currentArg(args[i]);

            if (Common::UtilityImpl::StringUtils::startswith(currentArg, "--central-group"))
            {
                updateConfigOptions(MCS::CENTRAL_GROUP, currentArg, configOptions);
            }
            else if (currentArg == "--customer-token")
            {
                if ((i + 1) < argSize)
                {
                    configOptions[MCS::MCS_CUSTOMER_TOKEN] = args[++i];
                }
            }
            else if (Common::UtilityImpl::StringUtils::startswith(currentArg, "--products"))
            {
                updateConfigOptions(MCS::MCS_PRODUCTS, currentArg, configOptions);
            }
            else if (currentArg == "--proxy-credentials")
            {
                if ((i + 1) < argSize)
                {
                    proxyCredentials = args[++i];
                }
            }
            else if (currentArg == "--message-relay")
            {
                if ((i + 1) < argSize)
                {
                    messageRelaysAsString = args[++i];
                }
            }
            else if (currentArg == "--version")
            {
                if ((i + 1) < argSize)
                {
                    configOptions[MCS::VERSION_NUMBER] = args[++i];
                }
            }
        }

        // default set of options, note these options are populated later after registration
        // defaulting here to make is clearer when they are being set.
        configOptions[MCS::MCS_ID] = "";
        configOptions[MCS::MCS_PASSWORD] = "";
        configOptions[MCS::MCS_PRODUCT_VERSION] = "";

        if (proxyCredentials.empty())
        {
            proxyCredentials = systemUtils->getEnvironmentVariable("PROXY_CREDENTIALS");
        }
        if (!proxyCredentials.empty())
        {
            std::vector<std::string> values = Common::UtilityImpl::StringUtils::splitString(proxyCredentials, ":");
            if (values.size() == 2)
            {
                configOptions[MCS::MCS_PROXY_USERNAME] = values[0];
                configOptions[MCS::MCS_PROXY_PASSWORD] = values[1];
                try
                {
                    configOptions[MCS::MCS_POLICY_PROXY_CREDENTIALS] =
                        Common::ObfuscationImpl::SECObfuscate(proxyCredentials);
                }
                catch (Common::Obfuscation::IObfuscationException& e)
                {
                    LOGWARN("Failed to use Proxy Credentials due to: " << e.what());
                    configOptions.erase(MCS::MCS_PROXY_USERNAME);
                    configOptions.erase(MCS::MCS_PROXY_PASSWORD);
                    configOptions.erase(MCS::MCS_POLICY_PROXY_CREDENTIALS);
                }
            }
            else
            {
                LOGWARN("Unable to parse Proxy Credentials as they do not follow <username:password> format.");
            }
        }

        std::string proxy = systemUtils->getEnvironmentVariable("https_proxy");
        if (proxy.empty())
        {
            proxy = systemUtils->getEnvironmentVariable("HTTPS_PROXY");
        }
        if (proxy.empty())
        {
            proxy = systemUtils->getEnvironmentVariable("http_proxy");
        }

        configOptions[MCS::MCS_PROXY] = proxy;
        if (allowedToUseMcsCertOverride(systemUtils))
        {
            configOptions[MCS::MCS_CA_OVERRIDE] = systemUtils->getEnvironmentVariable("MCS_CA");
        }

        std::vector<MCS::MessageRelay> messageRelays = extractMessageRelays(messageRelaysAsString);
        std::vector<MCS::MessageRelay> sortedMessageRelays = sortMessageRelays(messageRelays);

        return MCS::ConfigOptions{ sortedMessageRelays, configOptions };
    }

    MCS::ConfigOptions registerAndObtainMcsOptions(MCS::ConfigOptions& configOptions)
    {
        CentralRegistration centralRegistration;
        auto agentAdapter = std::make_shared<MCS::AgentAdapter>();
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(
            configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], client);

        centralRegistration.registerWithCentral(configOptions, mcsHttpClient, agentAdapter);
        // return updated configOptions
        return configOptions;
    }

    MCS::ConfigOptions innerCentralRegistration(const std::vector<std::string>& args, const std::string& mcsCertPath)
    {
        std::shared_ptr<OSUtilities::ISystemUtils> systemUtils = std::make_shared<OSUtilitiesImpl::SystemUtils>();

        MCS::ConfigOptions configOptions = processCommandLineOptions(args, systemUtils);
        if (configOptions.config.empty())
        {
            throw std::runtime_error("Failed to process command line options");
        }

        // When we call this function from the thin installer we need to specify the certificate we use in addition
        // to any command line args being passed in.
        if (!mcsCertPath.empty())
        {
            configOptions.config[MCS::MCS_CERT] = mcsCertPath;
        }

        return registerAndObtainMcsOptions(configOptions);
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup loggerSetup;
        // convert args to vector for to remove the need to handle pointers.
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i)
        {
            args.emplace_back(argv[i]);
        }
        try
        {
            innerCentralRegistration(args);
        }
        catch (std::runtime_error& ex)
        {
            LOGERROR(ex.what());
            return 1;
        }
        return 0;
    }

} // namespace CentralRegistration
