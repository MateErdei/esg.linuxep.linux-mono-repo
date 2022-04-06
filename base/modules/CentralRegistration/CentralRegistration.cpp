/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CentralRegistration.h"

#include "Logger.h"

#include <cmcsrouter/AgentAdapter.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/MCSApiCalls.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <json.hpp>

namespace CentralRegistrationImpl
{
    bool CentralRegistration::tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy)
    {
        MCS::MCSHttpClient httpClient(url, token, m_requester);

        httpClient.setCertPath(configOptions.config[MCS::MCS_CERT]);
        MCS::MCSApiCalls mcsApi;

        try
        {
            std::string preregistrationBody = mcsApi.preregisterEndpoint(httpClient, configOptions, statusXml, proxy);
            if(!preregistrationBody.empty())
            {
                std::string newMcsToken = processPreregistrationBody(preregistrationBody);

                if (!newMcsToken.empty())
                {
                    configOptions.config[MCS::MCS_TOKEN] = newMcsToken;
                    return true;
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Exception caught while attempting preregister: " << ex.what());
        }

        return false;
    }

    bool CentralRegistration::tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy)
    {
        MCS::MCSHttpClient httpClient(url, token, m_requester);
        httpClient.setCertPath(configOptions.config[MCS::MCS_CERT]);

        MCS::MCSApiCalls mcsApi;

        return mcsApi.registerEndpoint(httpClient, configOptions, statusXml, proxy);
    }

    bool CentralRegistration::tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token,
                                                         bool (*registrationFunction)(MCS::ConfigOptions&, const std::string&, std::string, std::string, std::string))
    {
        for(auto& messageRelay : configOptions.messageRelays) // These need to be ordered before this
        {
            std::string proxy = messageRelay.address + ":" + messageRelay.port;
            if(registrationFunction(configOptions, statusXml, url, token, proxy))
            {
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = proxy;
                return true;
            }
        }
        if(configOptions.config[MCS::MCS_CONNECTED_PROXY].empty() && !configOptions.config[MCS::MCS_PROXY].empty())
        {
            if(registrationFunction(configOptions, statusXml, url, token, configOptions.config[MCS::MCS_PROXY]))
            {
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = configOptions.config[MCS::MCS_PROXY];
                return true;
            }
        }
        return registrationFunction(configOptions, statusXml, url, token, "");
    }

    void CentralRegistration::preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml)
    {
        // check options are all there: customer token + selected products
        if(configOptions.config.empty() || configOptions.config[MCS::MCS_CUSTOMER_TOKEN].empty() || configOptions.config[MCS::MCS_PRODUCTS].empty())
        {
            return;
        }
        LOGINFO("Carrying out Preregistration for selected products: " << configOptions.config[MCS::MCS_PRODUCTS]);

        std::string url(configOptions.config[MCS::MCS_URL]);
        std::string token(configOptions.config[MCS::MCS_CUSTOMER_TOKEN]);

        if(!tryRegistrationWithProxies(configOptions, statusXml, url, token, tryPreregistration))
        {
            LOGINFO("Preregistration failed - continuing with default registration");
        }
    }

    std::string CentralRegistration::processPreregistrationBody(const std::string& preregistrationBody)
    {
        LOGDEBUG("\nPreregistrationBody:\n" << preregistrationBody << "\n\n");
        auto bodyAsJson = nlohmann::json::parse(preregistrationBody);
        std::string deploymentRegistrationToken = bodyAsJson["registrationToken"];
        if(deploymentRegistrationToken.empty())
        {
            LOGERROR("No MCS Token returned from Central - Preregistration request failed");
            return "";
        }

        for(auto& product : bodyAsJson["products"])
        {
            if(product["supported"].empty())
            {
                LOGWARN("Unsupported product chosen for preregistration: " << product);
            }
        }

        LOGDEBUG("Preregistration successful, new deployment registration token: " << deploymentRegistrationToken);
        return deploymentRegistrationToken;

    }

    void CentralRegistration::registerWithCentral(
            MCS::ConfigOptions& configOptions,
            std::shared_ptr<Common::HttpRequests::IHttpRequester> requester,
            std::shared_ptr<MCS::IAdapter> agentAdapter)
    {
        LOGDEBUG("Beginning product registration");
        m_requester = requester;
        std::string statusXml = agentAdapter->getStatusXml(configOptions.config);

        preregistration(configOptions, statusXml);

        std::string url(configOptions.config[MCS::MCS_URL]);
        std::string token(configOptions.config[MCS::MCS_TOKEN]);

        if(!configOptions.config[MCS::MCS_CONNECTED_PROXY].empty() && tryRegistration(configOptions, statusXml, url, token, configOptions.config[MCS::MCS_CONNECTED_PROXY]))
        {
            LOGINFO("Product successfully registered");
        }
        else if(tryRegistrationWithProxies(configOptions, statusXml, url, token, tryRegistration))
        {
            LOGINFO("Product successfully registered via proxy: " << configOptions.config[MCS::MCS_CONNECTED_PROXY]);
        }
        else
        {
            LOGERROR("Product registration failed");
        }
    }
}
