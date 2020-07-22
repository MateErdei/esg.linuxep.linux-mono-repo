/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <modules/CommsComponent/CommsConfig.h>
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

::testing::AssertionResult configAreEquivalent(
        std::stringstream & s,
        const CommsComponent::CommsConfig& expected,
        const CommsComponent::CommsConfig& actual)
{
    if (expected.getKeyList() != actual.getKeyList())
    {
        //s << "KeyList differs. expected: " << expected.getKeyList() << " actual " << actual.getKeyList();
        return ::testing::AssertionFailure() << s.str();
    }

//    actual.readCurrentProxyInfo();
//    expected.readCurrentProxyInfo();
//    std::cout << s.str() << std::endl;



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
    else if (std::holds_alternative<Common::HttpSender::HttpResponse>(expected.content))
    {
        return responseAreEquivalent(s, std::get<Common::HttpSender::HttpResponse>(expected.content), std::get<Common::HttpSender::HttpResponse>(actual.content));
    }
    else
    {
        return configAreEquivalent(s, std::get<CommsComponent::CommsConfig>(expected.content), std::get<CommsComponent::CommsConfig>(actual.content));
    }

}

CommsMsg serializeAndDeserialize(const CommsMsg& input)
{
    auto serialized = CommsComponent::CommsMsg::serialize(input);
    return CommsComponent::CommsMsg::fromString(serialized);
}

TEST_F(TestCommsMsg, DefaultCommsCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, IDofCommsCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    input.id = "stuff";
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(result.id,"stuff");
}

TEST_F(TestCommsMsg, bodyContentOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.bodyContent = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).bodyContent,"stuff");
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).bodyContent,httpResponse.bodyContent);
}

TEST_F(TestCommsMsg, httpCodeOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.httpCode = 400;
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).httpCode,400);
}

TEST_F(TestCommsMsg, descriptionOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.description = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).description,"stuff");
}

TEST_F(TestCommsMsg, bodyContentOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setData("stuff");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getData(),"stuff");
}
TEST_F(TestCommsMsg, requestTypeOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setRequestTypeFromString("GET");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getRequestTypeAsString(),"GET");
}

TEST_F(TestCommsMsg, HeadersOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    std::vector<std::string> headers;
    headers.push_back("header");
    headers.push_back("header1");
    requestConfig.setAdditionalHeaders(headers);
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getAdditionalHeaders(),headers);
}

TEST_F(TestCommsMsg, serverOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setServer("testserver.com");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getServer(),"testserver.com");
}

TEST_F(TestCommsMsg, resourcePathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setResourcePath("/blah/blah1");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getResourcePath(),"/blah/blah1");
}

TEST_F(TestCommsMsg, certPathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setCertPath("/tmp/cert.pem");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getCertPath(),"/tmp/cert.pem");
}

TEST_F(TestCommsMsg, portOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setPort(433);
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getPort(),433);
}

TEST_F(TestCommsMsg, keyValueOfConfigCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsConfig config;
    config.addKeyValueToList(std::pair<std::string,std::vector<std::string>>("stuff1",{"stuff"}));
    input.content = config;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    std::map<std::string, std::vector<std::string>> key_composed_values;
    key_composed_values.insert(std::pair<std::string,std::vector<std::string>>("stuff1",{"stuff"}));
    EXPECT_EQ(std::get<CommsComponent::CommsConfig>(result.content).getKeyList(),key_composed_values);
}