/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <CentralRegistration/CentralRegistration.h>
#include <cmcsrouter/Config.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockHttpRequester.h>
#include <tests/cmscrouter/MockAgentAdapter.h>
#include <Common/ObfuscationImpl/Base64.h>

class CentralRegistrationTests : public LogInitializedTests
{
    public:

    MCS::ConfigOptions basicMcsConfigOptions()
    {
        MCS::ConfigOptions configOptions;
        configOptions.config[MCS::MCS_URL] = "MCS_URL";
        configOptions.config[MCS::MCS_TOKEN] = "MCS_Token";
        configOptions.config[MCS::MCS_CERT] = "MCS_Cert";
        configOptions.config[MCS::MCS_ID] = "MCS_ID";
        configOptions.config[MCS::MCS_PASSWORD] = "MCS_Password";
        configOptions.config[MCS::MCS_PRODUCT_VERSION] = "MCS_Product_Version";
        return configOptions;
    }

    std::string basicXmlStatus{"XML content"};

    Common::HttpRequests::Response basicRegistrationResponse()
    {
        Common::HttpRequests::Response response;
        response.status = 200;
        response.body = Common::ObfuscationImpl::Base64::Encode("mcs_id:mcs_password");
        return response;
    }
};

TEST_F(CentralRegistrationTests, BasicRegistrationSuccessful) // NOLINT
{
    MCS::ConfigOptions configOptions = basicMcsConfigOptions();
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    // Mock Logger / logging interceptor

    // Set expectations
    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(basicRegistrationResponse()));

    CentralRegistrationImpl::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mockHttpRequester, mockAgentAdapter);
}