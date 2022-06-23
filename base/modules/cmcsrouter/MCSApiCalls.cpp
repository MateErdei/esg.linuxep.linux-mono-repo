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
    std::map<std::string,std::string> MCSApiCalls::getAuthenticationInfo(MCSHttpClient client)
    {
        std::map<std::string,std::string> list;

        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({"Content-Type","application/xml; charset=utf-8"});
        Common::HttpRequests::Response response =
            client.sendMessageWithIDAndRole("/authenticate/endpoint/",
                                      Common::HttpRequests::RequestType::POST,
                                          requestHeaders);
        if (response.status == 200)
        {
            nlohmann::json j;
            try
            {
                j = nlohmann::json::parse(response.body);
            }
            catch (nlohmann::json::parse_error& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json: " << response.body << " with error: " << ex.what();
                LOGWARN(errorMessage.str());
            }
            list.insert({"tenant_id",j.value("tenant_id","")});
            list.insert({"device_id",j.value("device_id","")});
            list.insert({"access_token",j.value("access_token","")});

        }
        return list;
    }

    std::string MCSApiCalls::preregisterEndpoint(
        MCSHttpClient& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        client.setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client.setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client.setCertPath(configOptions.config[MCS::MCS_CERT]);
        }
        
        client.setProxyInfo(proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response = client.sendPreregistration(statusXml, configOptions.config[MCS::MCS_CUSTOMER_TOKEN]);
        return response.body;
    }

    bool MCSApiCalls::registerEndpoint(
        MCSHttpClient& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        client.setID(configOptions.config[MCS::MCS_ID]);
        client.setPassword(configOptions.config[MCS::MCS_PASSWORD]);
        client.setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client.setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client.setCertPath(configOptions.config[MCS::MCS_CERT]);
        }
        
        client.setProxyInfo(proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response = client.sendRegistration(statusXml, configOptions.config[MCS::MCS_TOKEN]);
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
