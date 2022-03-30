/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CentralRegistration.h"

#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <CurlWrapper/CurlWrapper.h>
#include <cmcsrouter/AgentAdapter.h>
#include <cmcsrouter/MCSApiCalls.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <memory>

namespace CentralRegistrationImpl
{
    void CentralRegistration::RegisterWithCentral(std::map<std::string, std::string>& configOptions)
    {
        std::string url(configOptions["mcsUrl"]);
            //"https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep/register";
        std::string token(configOptions["mcsToken"]);

        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> requester
            = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(url, token, requester);

        MCS::MCSApiCalls mcsApi;
        mcsApi.registerEndpoint(httpClient, configOptions);
    }

}