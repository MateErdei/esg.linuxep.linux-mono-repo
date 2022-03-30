/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "MCSApiCalls.h"
#include "AgentAdapter.h"

#include <Common/ObfuscationImpl/Base64.h>

#include <sstream>

namespace MCS
{
    std::string getJwToken(MCSHttpClient client)
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

    void MCSApiCalls::registerEndpoint(MCSHttpClient& client, std::map<std::string, std::string>& registerConfig)
    {
        AgentAdapter agentAdapter;
        std::string mcsCertPath;
        std::string mcsId(registerConfig["mcsId"]);
        std::string password(registerConfig["password"]);
        std::string token(registerConfig["token"]);
        std::string productVersion(registerConfig["productVersion"]);

        std::string registrationXml = agentAdapter.getStatusXml();
        if(registrationXml.empty())
        {
            // handle errors.
            return;
        }

        std::stringstream authorisation;
        authorisation << mcsId << ":" << password << token;
        std::string encodedAuthorisation =
            Common::ObfuscationImpl::Base64::Encode(authorisation.str());

        std::string authorisationValue = "Basic " + encodedAuthorisation;
        std::string userAgentValue = "Sophos MCS Client/" + productVersion + "Linux sessions regToken";

        Common::HttpRequests::Headers headers = {
            {"Authorization", authorisationValue},
            {"Content-Type","application/xml; charset=utf-8"}
           // {"User-Agent",userAgentValue}
        };
//        Common::HttpRequests::Headers headers = {
//            {"Authorization","Basic OjpjOGE2YTQ1MmFiZmM1MDBmZWU1ZDZmYzgzMjFlOTVlNWY1MzVlMTZiZTlkMGY1MWJlMDk0MDJkYTkzYTY1MGZk"},
//
//            {"User-Agent","Sophos MCS Client/1.1.9.9999 Linux sessions regToken"}
//        };

        client.setCertPath(mcsCertPath);
        Common::HttpRequests::Response response = client.sendRegistration(headers);
        if(response.status == 200)
        {
            std::string messageBody = Common::ObfuscationImpl::Base64::Decode(response.body);

        }

//        auto response = client.post(  Common::HttpRequests::RequestConfig {
//            .url=url,
//            .headers = headers,
//            .data=registrationXml,
//            .certPath = certPath});
//        std::cout << "Body:" << response.body << std::endl;
//        std::cout << "Status:" << response.status << std::endl;
//        std::cout << "Headers:" << std::endl;
//        for (auto const& [header, value]:  response.headers)
//        {
//            std::cout << header<<":" << value << std::endl;
//        }
//        // Expected reply:
//        ASSERT_EQ(response.body, "YjZlNjg1MzUtMmQyMy1lNDUyLTlhZjYtMjY5ZmMwNWMwYjBhOnoxbUxaQnJJSlhHOVprWGw=");
//        ASSERT_EQ(response.status, 200);

    }
}
