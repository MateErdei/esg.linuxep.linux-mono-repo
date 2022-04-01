/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "MCSApiCalls.h"
#include "AgentAdapter.h"
#include "Logger.h"
#include <Common/ObfuscationImpl/Base64.h>

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
        std::string customerToken = registerConfig["customerToken"];
        std::string encodedCustomerToken = Common::ObfuscationImpl::Base64::Encode(customerToken);

        Common::HttpRequests::Headers headers = {
            {"Authorization", encodedCustomerToken},
            {"Content-Type","application/xml; charset=utf-8"}
        };

        Common::HttpRequests::Response response = client.sendRegistration(headers, "/install/deployment-info/2", statusXml);
        return response.body;
    }

    void MCSApiCalls::registerEndpoint(
        MCSHttpClient& client,
        std::map<std::string, std::string>& registerConfig,
        const std::string& statusXml)
    {
        AgentAdapter agentAdapter;

        std::string mcsId = registerConfig["mcsId"];
        std::string password = registerConfig["password"];
        std::string token(registerConfig["mcsToken"]);

        std::string productVersion(registerConfig["productVersion"]);

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
        }
        else
        {
            std::cout << "Error during registration" << response.status << std::endl;
        }
    }
}
