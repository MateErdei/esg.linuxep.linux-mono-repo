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
    std::string expectedContent = CommsComponent::CommsMsg::serialize(expected);
    std::string actualContent = CommsComponent::CommsMsg::serialize(actual);
    if (actualContent != expectedContent)
    {
        std::cout << "Expected content: " << expectedContent << std::endl;
        std::cout << "Actual content: " << actualContent << std::endl;
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