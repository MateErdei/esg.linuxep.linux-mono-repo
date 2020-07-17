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
    // change something.
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}
