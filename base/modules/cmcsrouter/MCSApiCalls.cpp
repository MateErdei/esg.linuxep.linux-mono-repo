/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "MCSApiCalls.h"
#include "AgentAdapter.h"
#include "Config.h"
#include "Logger.h"
#include <Common/ObfuscationImpl/Base64.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <sstream>

namespace MCS
{
    std::string MCSApiCalls::getJwToken(MCSHttpClient client)
    {
        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({"Content-Type","application/xml; charset=utf-8"});
        Common::HttpRequests::Response response =
            client.sendMessageWithIDAndRole("/authenticate/endpoint/",
                                      Common::HttpRequests::RequestType::POST,
                                          requestHeaders);
        if (response.status == 200)
        {
            return response.body;
        }
        return "";
    }

    std::string MCSApiCalls::preregisterEndpoint(
        MCSHttpClient& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        std::string encodedCustomerToken = "Basic " + Common::ObfuscationImpl::Base64::Encode(configOptions.config[MCS::MCS_CUSTOMER_TOKEN]);
        client.setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        Common::HttpRequests::Headers headers = {
            {"Authorization", encodedCustomerToken},
            {"Content-Type","application/json;charset=UTF-8"}
        };

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client.setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client.setCertPath(configOptions.config[MCS::MCS_CERT]);
        }
        
        client.setProxyInfo(proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response = client.sendRegistration(headers, "/install/deployment-info/2", statusXml);
        return response.body;
    }

    bool MCSApiCalls::registerEndpoint(
        MCSHttpClient& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        AgentAdapter agentAdapter;

        std::string encodedAuthorisation =
            Common::ObfuscationImpl::Base64::Encode(
                configOptions.config[MCS::MCS_ID] + ":" +  configOptions.config[MCS::MCS_PASSWORD] + ":" + configOptions.config[MCS::MCS_TOKEN]);

        std::string authorisationValue = "Basic " + encodedAuthorisation;
        client.setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        Common::HttpRequests::Headers headers = {
            {"Authorization", authorisationValue},
            {"Content-Type","application/xml; charset=utf-8"}
        };

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client.setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client.setCertPath(configOptions.config[MCS::MCS_CERT]);
        }
        
        client.setProxyInfo(proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response = client.sendRegistration(headers, "/register", statusXml);
        if (response.status == 200)
        {
            std::string messageBody = Common::ObfuscationImpl::Base64::Decode(response.body);
            std::vector<std::string> responseValues = Common::UtilityImpl::StringUtils::splitString(messageBody, ":");
            if (responseValues.size() == 2)
            {
                // Note that updating the configOptions here should be propagated back to the caller as it is all
                // passed by reference.
                configOptions.config[MCS::MCS_ID] = responseValues[0]; // endpointId
                configOptions.config[MCS::MCS_PASSWORD] = responseValues[1]; //MCS Password
                return true;
            }
        }
        else
        {
            LOGWARN("Error during registration: " << response.status);
        }
        return false;
    }
}
