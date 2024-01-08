// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockAgentAdapter.h"

#include "CentralRegistration/CentralRegistration.h"
#include "Common/ObfuscationImpl/Base64.h"
#include "cmcsrouter/Config.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>

class CentralRegistrationTests : public LogInitializedTests
{
public:
    void SetUp() override
    {
        setenv("SOPHOS_TEMP_DIRECTORY", "/tmp", 1);
    }
    
    void TearDown() override
    {
        unsetenv("SOPHOS_TEMP_DIRECTORY");
    }

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

    MCS::ConfigOptions addProxyToConfigOptions(MCS::ConfigOptions configOptions)
    {
        configOptions.config[MCS::MCS_PROXY] = "MCS_Proxy";
        configOptions.config[MCS::MCS_PROXY_USERNAME] = "MCS_Proxy_Username";
        configOptions.config[MCS::MCS_PROXY_PASSWORD] = "MCS_Proxy_Password";
        return configOptions;
    }

    MCS::ConfigOptions addMessageRelaysToConfigOptions(MCS::ConfigOptions configOptions)
    {
        std::vector<MCS::MessageRelay> messageRelays(
            { { 0, "Relay1", "Address1", "Port1" }, { 1, "Relay2", "Address2", "Port2" } });
        configOptions.messageRelays = messageRelays;
        return configOptions;
    }

    MCS::ConfigOptions addPreregistrationToConfigOptions(MCS::ConfigOptions configOptions)
    {
        configOptions.config[MCS::MCS_CUSTOMER_TOKEN] = "MCS_Customer_Token";
        configOptions.config[MCS::MCS_PRODUCTS] = "MCS_Product";
        return configOptions;
    }

    std::string basicXmlStatus{"XML content"};
    std::string registerUrlSuffix{"/register"};
    std::string preregisterUrlSuffix{"/install/deployment-info/2"};

    Common::HttpRequests::RequestConfig createRequestFromConfigOptions(MCS::ConfigOptions configOptions, std::string statusXml, bool withPreregistration=false)
    {
        if(withPreregistration) { configOptions.config[MCS::MCS_TOKEN] = "New_MCS_Token"; }
        Common::HttpRequests::Headers headers = {
            {"User-Agent", "Sophos MCS Client/" + configOptions.config[MCS::MCS_PRODUCT_VERSION] + " Linux sessions "+ configOptions.config[MCS::MCS_TOKEN]},
            {"Authorization", "Basic " + Common::ObfuscationImpl::Base64::Encode(configOptions.config[MCS::MCS_ID] + ":" + configOptions.config[MCS::MCS_PASSWORD] + ":" + configOptions.config[MCS::MCS_TOKEN])},
            {"Content-Type","application/xml; charset=utf-8"}
        };
        Common::HttpRequests::RequestConfig request{ .url = configOptions.config[MCS::MCS_URL] + registerUrlSuffix, .headers = headers, .data = statusXml, .certPath = configOptions.config[MCS::MCS_CERT], .timeout = 60};
        return request;
    }

    Common::HttpRequests::RequestConfig createPreregistrationRequestFromConfigOptions(MCS::ConfigOptions configOptions, std::string statusXml)
    {
        Common::HttpRequests::Headers headers = {
            {"User-Agent", "Sophos MCS Client/" + configOptions.config[MCS::MCS_PRODUCT_VERSION] + " Linux sessions "+ configOptions.config[MCS::MCS_CUSTOMER_TOKEN]},
            {"Authorization", "Basic " + Common::ObfuscationImpl::Base64::Encode(configOptions.config[MCS::MCS_CUSTOMER_TOKEN])},
            {"Content-Type","application/json;charset=UTF-8"}
        };
        Common::HttpRequests::RequestConfig request{ .url = configOptions.config[MCS::MCS_URL] + preregisterUrlSuffix, .headers = headers, .data = statusXml, .certPath = configOptions.config[MCS::MCS_CERT], .timeout = 60};
        return request;
    }

    Common::HttpRequests::RequestConfig addProxyDetailsToRequestFromConfig(Common::HttpRequests::RequestConfig request, std::string proxy, std::string proxyUsername, std::string proxyPassword)
    {
        request.proxy = proxy;
        request.proxyUsername = proxyUsername;
        request.proxyPassword = proxyPassword;
        return request;
    }

        Common::HttpRequests::Response basicRegistrationResponseSuccess()
    {
        Common::HttpRequests::Response response;
        response.status = 200;
        response.body = Common::ObfuscationImpl::Base64::Encode("mcs_id:mcs_password");
        response.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
        return response;
    }

    Common::HttpRequests::Response basicRegistrationResponseFailure()
    {
        Common::HttpRequests::Response response;
        response.status = 400;
        response.error = "Test 400 Error";
        return response;
    }

    Common::HttpRequests::Response addPreregistrationTokenWithSupportedProductsToResponse(Common::HttpRequests::Response response)
    {
        response.body = R"({ "registrationToken" : "New_MCS_Token", "products" : [ { "product" : "MCS_Product", "supported" : true } ] })";
        return response;
    }

    Common::HttpRequests::Response addPreregistrationTokenWithUnsupportedProductsToResponse(Common::HttpRequests::Response response)
    {
        response.body = R"({ "registrationToken" : "New_MCS_Token", "products" : [ { "product" : "MCS_Product", "supported" : false } ] })";
        return response;
    }

    Common::HttpRequests::Response addMalformedPreregistrationResponse(Common::HttpRequests::Response response)
    {
        response.body = R"({ "products" : [ { "product" : "MCS_Product", "supported" : false } ] })";
        return response;
    }
};

TEST_F(CentralRegistrationTests, BasicRegistrationSucceeds)
{
    MCS::ConfigOptions configOptions = basicMcsConfigOptions();
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus))).WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationFails)
{
    MCS::ConfigOptions configOptions = basicMcsConfigOptions();
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseFailure()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::HasSubstr("Connection error during registration going direct: Test 400"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product registration failed"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithProxySucceeds)
{
    MCS::ConfigOptions configOptions = addProxyToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(
                                        addProxyDetailsToRequestFromConfig(
                                            createRequestFromConfigOptions(
                                                configOptions,
                                                basicXmlStatus),
                                            configOptions.config[MCS::MCS_PROXY],
                                            configOptions.config[MCS::MCS_PROXY_USERNAME],
                                            configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseSuccess()));
    std::string expected_ini = "usedProxy = true\nusedUpdateCache = false\nusedMessageRelay = false\nproxyOrMessageRelayURL = "+configOptions.config[MCS::MCS_PROXY]+'\n';

    EXPECT_CALL(*mockFileSystem, writeFile("/tmp/registration_comms_check.ini", expected_ini)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config[MCS::MCS_CONNECTED_PROXY] == configOptions.config[MCS::MCS_PROXY]);
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered via proxy: MCS_Proxy"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithProxyFails)
{
    MCS::ConfigOptions configOptions = addProxyToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.config[MCS::MCS_PROXY],
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
            configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseFailure()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::HasSubstr("Connection error during registration going direct: Test 400"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product registration failed"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithFailsOnProxiesAndThenDirectSucceeds)
{
    MCS::ConfigOptions configOptions = addMessageRelaysToConfigOptions(addProxyToConfigOptions(basicMcsConfigOptions()));
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[0].address + ":" + configOptions.messageRelays[0].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[1].address + ":" + configOptions.messageRelays[1].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.config[MCS::MCS_PROXY],
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_FALSE(configOptions.config[MCS::MCS_CONNECTED_PROXY] == configOptions.config[MCS::MCS_PROXY]);
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: Address1:Port1")));
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: Address2:Port2")));
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: MCS_Proxy")));
    ASSERT_THAT(
        logMessage,
        ::testing::HasSubstr("Connection error during registration via proxy Address1:Port1: Test 400 Error"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithMessageRelaySucceeds)
{
    MCS::ConfigOptions configOptions = addMessageRelaysToConfigOptions(addProxyToConfigOptions(basicMcsConfigOptions()));
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[0].address + ":" + configOptions.messageRelays[0].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config[MCS::MCS_CONNECTED_PROXY] == "Address1:Port1");
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered via proxy: Address1:Port1"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithMessageRelaysSucceedsOnSecondRelay)
{
    MCS::ConfigOptions configOptions = addMessageRelaysToConfigOptions(addProxyToConfigOptions(basicMcsConfigOptions()));
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[0].address + ":" + configOptions.messageRelays[0].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[1].address + ":" + configOptions.messageRelays[1].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
            configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config[MCS::MCS_CONNECTED_PROXY] == "Address2:Port2");
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: Address1:Port1")));
    ASSERT_THAT(
        logMessage,
        ::testing::HasSubstr("Connection error during registration via proxy Address1:Port1: Test 400 Error"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered via proxy: Address2:Port2"));
}

TEST_F(CentralRegistrationTests, BasicRegistrationWithMessageRelaysFailsThenProxySucceeds)
{
    MCS::ConfigOptions configOptions = addMessageRelaysToConfigOptions(addProxyToConfigOptions(basicMcsConfigOptions()));
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[0].address + ":" + configOptions.messageRelays[0].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.messageRelays[1].address + ":" + configOptions.messageRelays[1].port,
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(addProxyDetailsToRequestFromConfig(
                                        createRequestFromConfigOptions(configOptions, basicXmlStatus),
                                        configOptions.config[MCS::MCS_PROXY],
                                        configOptions.config[MCS::MCS_PROXY_USERNAME],
                                        configOptions.config[MCS::MCS_PROXY_PASSWORD])))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config[MCS::MCS_CONNECTED_PROXY] == configOptions.config[MCS::MCS_PROXY]);
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: Address1:Port1")));
    ASSERT_THAT(
        logMessage, ::testing::Not(::testing::HasSubstr("Product successfully registered via proxy: Address2:Port2")));
    ASSERT_THAT(
        logMessage,
        ::testing::HasSubstr("Connection error during registration via proxy Address1:Port1: Test 400 Error"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered via proxy: MCS_Proxy"));
}

TEST_F(CentralRegistrationTests, PreregistrationSucceeds)
{
    MCS::ConfigOptions configOptions = addPreregistrationToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createPreregistrationRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(addPreregistrationTokenWithSupportedProductsToResponse(basicRegistrationResponseSuccess())));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus, true)))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("Unsupported product chosen for preregistration: ")));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Preregistration successful, new deployment registration token: New_MCS_Token"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}

TEST_F(CentralRegistrationTests, PreregistrationFailsButProductStillRegisters)
{
    MCS::ConfigOptions configOptions = addPreregistrationToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createPreregistrationRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseFailure()));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("Preregistration successful, new deployment registration token: ")));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Preregistration failed - continuing with default registration"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}

TEST_F(CentralRegistrationTests, PreregistrationSucceedsWithUnsupportedProducts)
{
    MCS::ConfigOptions configOptions = addPreregistrationToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createPreregistrationRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(addPreregistrationTokenWithUnsupportedProductsToResponse(basicRegistrationResponseSuccess())));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus, true)))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("Preregistration failed - continuing with default registration")));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Unsupported product chosen for preregistration: \"MCS_Product\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Preregistration successful, new deployment registration token: New_MCS_Token"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}

TEST_F(CentralRegistrationTests, PreregistrationFailsWhenNoNewTokenReturnedButStillRegisters)
{
    MCS::ConfigOptions configOptions = addPreregistrationToConfigOptions(basicMcsConfigOptions());
    auto mockAgentAdapter = std::make_shared<StrictMock<MockAgentAdapter>>();
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto mcsHttpClient = std::make_shared<MCS::MCSHttpClient>(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_TOKEN], mockHttpRequester);
    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockAgentAdapter, getStatusXml(configOptions.config)).WillOnce(Return(basicXmlStatus));
    EXPECT_CALL(*mockHttpRequester, post(createPreregistrationRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(addMalformedPreregistrationResponse(basicRegistrationResponseSuccess())));
    EXPECT_CALL(*mockHttpRequester, post(createRequestFromConfigOptions(configOptions, basicXmlStatus)))
        .WillOnce(Return(basicRegistrationResponseSuccess()));

    CentralRegistration::CentralRegistration centralRegistration;
    centralRegistration.registerWithCentral(configOptions, mcsHttpClient, mockAgentAdapter);

    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("Preregistration successful, new deployment registration token: ")));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("No MCS Token returned from Central - Preregistration request failed"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Product successfully registered"));
}