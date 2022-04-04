/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CentralRegistration.h"

#include "Logger.h"

#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <CurlWrapper/CurlWrapper.h>
#include <cmcsrouter/AgentAdapter.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/MCSApiCalls.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <cstdlib>
#include <json.hpp>
#include <memory>

namespace CentralRegistrationImpl
{
    bool CentralRegistration::tryPreregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy)
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> requester
            = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(url, token, requester);

        httpClient.setCertPath(configOptions.config[MCS::MCS_CERT]);
        MCS::MCSApiCalls mcsApi;

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
        return false;
    }

    bool CentralRegistration::tryRegistration(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token, std::string proxy)
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> requester
            = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(url, token, requester);
        httpClient.setCertPath(configOptions.config[MCS::MCS_CERT]);

        MCS::MCSApiCalls mcsApi;

        return mcsApi.registerEndpoint(httpClient, configOptions, statusXml, proxy);
    }

    void CentralRegistration::preregistration(MCS::ConfigOptions& configOptions, const std::string& statusXml)
    {
        // check options are all there: customer token + selected products
        if(configOptions.config.empty() && configOptions.config[MCS::MCS_CUSTOMER_TOKEN].empty() && configOptions.config[MCS::MCS_PRODUCTS].empty())
        {
            return;
        }

        std::string url(configOptions.config[MCS::MCS_URL]);
        std::string token(configOptions.config[MCS::MCS_CUSTOMER_TOKEN]);

        tryRegistrationWithProxies(configOptions, statusXml, url, token,
                                   tryPreregistration);
    }

    std::string CentralRegistration::processPreregistrationBody(const std::string& preregistrationBody)
    {
        LOGDEBUG("\nPreregistrationBody:\n" << preregistrationBody << "\n\n");
        auto bodyAsJson = nlohmann::json::parse(preregistrationBody);
        std::string deploymentRegistrationToken = bodyAsJson["registrationToken"];
        if(deploymentRegistrationToken.empty())
        {
            LOGERROR("Preregistration request failed - continuing with default registration");
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

    void CentralRegistration::RegisterWithCentral(MCS::ConfigOptions& configOptions)
    {
        MCS::AgentAdapter agentAdapter;
        std::string statusXml = agentAdapter.getStatusXml(configOptions.config);

        preregistration(configOptions, statusXml);

        std::string url(configOptions.config[MCS::MCS_URL]);
        std::string token(configOptions.config[MCS::MCS_TOKEN]);

        if(configOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
        {
            tryRegistrationWithProxies(configOptions, statusXml, url, token,
                                       tryRegistration);
        }
        else
        {
            tryRegistration(configOptions, statusXml, url, token, "");
        }
    }

    void CentralRegistration::tryRegistrationWithProxies(MCS::ConfigOptions& configOptions, const std::string& statusXml, std::string url, std::string token,
                bool (*func)(MCS::ConfigOptions&, const std::string&, std::string, std::string, std::string))
    {
        for(auto& messageRelay : configOptions.messageRelays) // These need to be ordered before this
        {
            std::string proxy = messageRelay.address + ":" + messageRelay.port;
            if(func(configOptions, statusXml, url, token, proxy))
            {
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = proxy;
                break;
            }
        }
        if(configOptions.config[MCS::MCS_CONNECTED_PROXY].empty() && !configOptions.config[MCS::MCS_PROXY].empty())
        {
            if(func(configOptions, statusXml, url, token, configOptions.config[MCS::MCS_PROXY]))
            {
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = configOptions.config[MCS::MCS_PROXY];
            }
        }
        if(configOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
        {
            func(configOptions, statusXml, url, token, "");
        }
    }
}
