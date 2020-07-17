/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

using namespace CommsComponent;

class TestCommsMsg : public LogOffInitializedTests
{
};

::testing::AssertionResult requestAreEquivalent(
    std::stringstream & s,
        const Common::HttpSender::RequestConfig& expected,
        const Common::HttpSender::RequestConfig& actual)
{
    if( expected.getAdditionalHeaders() != actual.getAdditionalHeaders())
    {
        s << "headers differ. Expected: " << expected.getAdditionalHeaders().size() <<  " Actual: " << actual.getAdditionalHeaders().size(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getCertPath() != actual.getCertPath())
    {
        s << "getCertPath differ. Expected: " << expected.getCertPath() <<  " Actual: " << actual.getCertPath(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getData() != actual.getData())
    {
        s << "getData differ. Expected: " << expected.getData() <<  " Actual: " << actual.getData(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getPort() != actual.getPort())
    {
        s << "getPort differ. Expected: " << expected.getPort() <<  " Actual: " << actual.getPort(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getRequestType() != actual.getRequestType())
    {
        s << "getRequestType differ. Expected: " << expected.getRequestTypeAsString() <<  " Actual: " << actual.getRequestTypeAsString(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getResourcePath() != actual.getResourcePath())
    {
        s << "getResourcePath differ. Expected: " << expected.getResourcePath() <<  " Actual: " << actual.getResourcePath(); 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if( expected.getServer() != actual.getServer())
    {
        s << "getServer differ. Expected: " << expected.getServer() <<  " Actual: " << actual.getServer(); 
        return ::testing::AssertionFailure() << s.str(); 
    }

    return ::testing::AssertionSuccess(); 
}

::testing::AssertionResult responseAreEquivalent(
    std::stringstream & s,
        const Common::HttpSender::HttpResponse& expected,
        const Common::HttpSender::HttpResponse& actual)
{
    if (expected.httpCode != actual.httpCode)
    {
        s << "httpCode differs. expected: " << expected.httpCode << " actual " << actual.httpCode; 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if (expected.description != actual.description)
    {
        s << "description differs. expected: " << expected.description << " actual " << actual.description; 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if (expected.bodyContent != actual.bodyContent)
    {
        s << "bodyContent differs. expected: " << expected.bodyContent << " actual " << actual.bodyContent; 
        return ::testing::AssertionFailure() << s.str(); 
    }


    return ::testing::AssertionSuccess(); 
}


::testing::AssertionResult commsMsgAreEquivalent(
        const char* m_expr,
        const char* n_expr,
        const CommsComponent::CommsMsg& expected,
        const CommsComponent::CommsMsg& actual) {
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";

    if (actual.id != expected.id)
    {
        return ::testing::AssertionFailure() << s.str() << "the ids are not the same";
    }
    if (expected.content.index() != actual.content.index())
    {
        s << "Not holding the same content variant"; 
        return ::testing::AssertionFailure() << s.str(); 
    }
    if (std::holds_alternative<Common::HttpSender::RequestConfig>(expected.content))
    {
        return requestAreEquivalent(s, std::get<Common::HttpSender::RequestConfig>(expected.content), std::get<Common::HttpSender::RequestConfig>(actual.content)); 
    }
    else
    {
        return responseAreEquivalent(s, std::get<Common::HttpSender::HttpResponse>(expected.content), std::get<Common::HttpSender::HttpResponse>(actual.content)); 
    }

    std::string expectedContent = CommsComponent::CommsMsg::serialize(expected);
    std::string actualContent = CommsComponent::CommsMsg::serialize(actual);
    if (actualContent != expectedContent)
    {
        s << "Expected content: " << expectedContent << "\n";
        s << "Actual content: " << actualContent << "\n";
        return ::testing::AssertionFailure() << s.str() << "the contents are not the same";
    }

    return ::testing::AssertionSuccess();
}

CommsMsg serializeAndDeserialize(const CommsMsg& input)
{
    auto serialized = CommsComponent::CommsMsg::serialize(input);
    return CommsComponent::CommsMsg::fromString(serialized);
}

TEST_F(TestCommsMsg, DefaultCommsCanGoThroughSerializationAndDeserialization) // NOLINT
{
    CommsMsg input;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, IDofCommsIsparsedCorrecltThroughtSerializationAndDeserialization) // NOLINT
{
    CommsMsg input;
    input.id = "stuff";
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, bodyContentOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.bodyContent = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, httpCodeOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.httpCode = 400;
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, descriptionOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.description = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, bodyContentOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setData("stuff");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, serverOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setServer("testserver.com");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, resourcePathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setResourcePath("/blah/blah1");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, certPathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setCertPath("/tmp/cert.pem");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, portOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setPort(433);
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}


TEST_F(TestCommsMsg, serializationAndDeserializationOfJsonShouldWorkForHttpResponse) // NOLINT
{
    Common::HttpSender::HttpResponse httpResponse; 
    httpResponse.description = "desc"; 
    httpResponse.httpCode = 3; 
    httpResponse.bodyContent = "body"; 
    CommsMsg input; 
    input.content = httpResponse; 
    Common::HttpSender::HttpResponse resHttp = CommsMsg::httpResponseFromJson(CommsMsg::toJson(httpResponse)); 
    CommsMsg result; 
    result.content = resHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, deserializeFromJsonForHttpResponse) // NOLINT
{
    Common::HttpSender::HttpResponse httpResponse; 
    httpResponse.description = "desc"; 
    httpResponse.httpCode = 3; 
    httpResponse.bodyContent = "body"; 
    std::string json = R"({
 "httpCode": "3",
 "description": "desc",
 "bodyContent": "body"
})"; 
    Common::HttpSender::HttpResponse resHttp = CommsMsg::httpResponseFromJson(json); 
    CommsMsg input;     
    CommsMsg result; 
    input.content = httpResponse; 
    result.content = resHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, serializationAndDeserializationOfJsonShouldWorkForHttpRequest) // NOLINT
{
    Common::HttpSender::RequestConfig configRequest{
        Common::HttpSender::RequestType::POST, 
        std::vector<std::string>{"header1","header2"}, 
        "domain.com",
        335,
        "certPath", "/index.html"}; 
    configRequest.setData("data"); 
    CommsMsg input; 
    input.content = configRequest; 
    Common::HttpSender::RequestConfig reqHttp = CommsMsg::requestConfigFromJson(CommsMsg::toJson(configRequest)); 

    // the certificatePath is not present in the serialization (do we want?)
    // FIXME: The issue would be that it would open the capability to 'exploit' by providing different certs. 
    // but allow for cert-pinning.    
    reqHttp.setCertPath( configRequest.getCertPath()); 

    CommsMsg result; 
    result.content = reqHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, deserializeFromJsonForConfigRequest) // NOLINT
{
    Common::HttpSender::RequestConfig configRequest{
        Common::HttpSender::RequestType::POST, 
        std::vector<std::string>{"header1","header2"}, 
        "domain.com",
        335,
        "", "/index.html"}; 
        // Observe that the certPath is empty.
    configRequest.setData("data"); 
    std::string json = R"({
 "requestType": "POST",
 "server": "domain.com",
 "resourcePath": "/index.html",
 "bodyContent": "data",
 "port": "335",
 "headers": [
  "header1",
  "header2"
 ]
})"; 
    Common::HttpSender::RequestConfig reqHttp = CommsMsg::requestConfigFromJson(json); 
    CommsMsg input;     
    CommsMsg result; 
    input.content = configRequest; 
    result.content = reqHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}
