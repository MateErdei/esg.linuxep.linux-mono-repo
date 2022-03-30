/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CentralRegistration.h"

#include <CurlWrapper/CurlWrapper.h>
#include <cmcsrouter/AgentAdapter.h>
#include <cmcsrouter/MCSApiCalls.h>
#include <cmcsrouter/MCSHttpClient.h>

#include <memory>

namespace CentralRegistrationImpl
{
    void CentralRegistration::RegisterWithCentral()
    {
        std::string url="https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep/register";
        std::string token; //token needs to be passed in?
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        MCS::MCSHttpClient httpClient(url, token, curlWrapper);

        MCS::MCSApiCalls mcsApi;
        mcsApi.registerEndpoint(httpClient);
    }

}