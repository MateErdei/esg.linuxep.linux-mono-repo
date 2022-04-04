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
        std::map<std::string, std::string>& registerConfig,
        const std::string& statusXml)
    {
        std::string customerToken = registerConfig[MCS::MCS_CUSTOMER_TOKEN];
        std::string encodedCustomerToken = "Basic " + Common::ObfuscationImpl::Base64::Encode(customerToken);

        Common::HttpRequests::Headers headers = {
            {"Authorization", encodedCustomerToken},
            {"Content-Type","application/json;charset=UTF-8"}
        };

        Common::HttpRequests::Response response = client.sendRegistration(headers, "/install/deployment-info/2", statusXml);
        std::cout << "\nError string: " << response.error << "\n\n";
        return response.body;
    }

    void MCSApiCalls::registerEndpoint(
        MCSHttpClient& client,
        std::map<std::string, std::string>& configOptions,
        const std::string& statusXml)
    {
        AgentAdapter agentAdapter;

        std::string mcsId = configOptions[MCS::MCS_ID];
        std::string password = configOptions[MCS::MCS_PASSWORD];
        std::string token(configOptions[MCS::MCS_TOKEN]);

        std::string productVersion(configOptions[MCS::MCS_PRODUCT_VERSION]);

        std::stringstream authorisation;
        authorisation << mcsId << ":" << password << ":" << token;

        std::string encodedAuthorisation =
            Common::ObfuscationImpl::Base64::Encode(authorisation.str());

        std::string authorisationValue = "Basic " + encodedAuthorisation;
        client.setVersion(productVersion);

        Common::HttpRequests::Headers headers = {
            {"Authorization", authorisationValue},
            {"Content-Type","application/xml; charset=utf-8"}
        };

        Common::HttpRequests::Response response = client.sendRegistration(headers, "/register", statusXml);
        if(response.status == 200)
        {
            std::string messageBody = Common::ObfuscationImpl::Base64::Decode(response.body);
            std::vector<std::string> responseValues = Common::UtilityImpl::StringUtils::splitString(messageBody, ":");
            if(responseValues.size() == 2)
            {
                // Note that updating the configOptions here should be propergated back to the caller as it is all
                // passed by reference.
                configOptions[MCS::MCS_ID] = responseValues[0]; // endpointId
                configOptions[MCS::MCS_PASSWORD] = responseValues[1]; //MCS Password
            }
        }
        else
        {
            std::cout << "Error during registration" << response.status << std::endl;
        }
    }
}
