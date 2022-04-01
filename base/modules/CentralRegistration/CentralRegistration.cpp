/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CentralRegistration.h"
#include "Logger.h"
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <CurlWrapper/CurlWrapper.h>
#include <cmcsrouter/AgentAdapter.h>
#include <cmcsrouter/MCSApiCalls.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <json.hpp>
#include <cstdlib>
#include <memory>

namespace CentralRegistrationImpl
{
    void CentralRegistration::preregistration(std::map<std::string, std::string>& configOptions, const std::string& statusXml, const std::string& mcsCert)
    {
        // check options are all there: customer token + selected products
        if(configOptions.empty() && configOptions["customerToken"].empty() && configOptions["products"].empty())
        {
            return;
        }

        std::string url(configOptions["mcsUrl"]);
        std::string token(configOptions["customerToken"]);

        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> requester
            = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(url, token, requester);

        httpClient.setCertPath(mcsCert);
        MCS::MCSApiCalls mcsApi;

        std::string preregistrationBody = mcsApi.preregisterEndpoint(httpClient, configOptions, statusXml);
        if(!preregistrationBody.empty())
        {
            std::string newMcsToken = processPreregistrationBody(preregistrationBody);

            if (!newMcsToken.empty())
            {
                configOptions["mcsToken"] = newMcsToken;
            }
        }
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

    void CentralRegistration::RegisterWithCentral(std::map<std::string, std::string>& configOptions)
    {
        MCS::AgentAdapter agentAdapter;
        std::string statusXml = agentAdapter.getStatusXml(configOptions);

        const char * val = std::getenv("MCS_CA");
        std::string mcsCert(val);

        preregistration(configOptions, statusXml, mcsCert);

        std::string url(configOptions["mcsUrl"]);
            //"https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep/register";
        std::string token(configOptions["mcsToken"]);


        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> requester
            = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(url, token, requester);
        httpClient.setCertPath(mcsCert);

        MCS::MCSApiCalls mcsApi;

        mcsApi.registerEndpoint(httpClient, configOptions, statusXml);
    }

}