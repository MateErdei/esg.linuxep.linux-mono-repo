// Copyright 2023 Sophos All rights reserved.

#include "Common/Policy/SerialiseUpdateSettings.h"
#include "Policy/PolicyParseException.h"
#include "tests/Common/Helpers/MemoryAppender.h"

namespace
{
    class TestSerialiseUpdateSettings : public MemoryAppenderUsingTests
    {
    public:
        TestSerialiseUpdateSettings() : MemoryAppenderUsingTests("Policy")
        {}
    };
}

using namespace Common::Policy;


TEST_F(TestSerialiseUpdateSettings, invalidJsonString)
{
    try
    {
        SerialiseUpdateSettings::fromJsonSettings("non json string");
        FAIL();
    }
    catch (const PolicyParseException& e)
    {
        EXPECT_STREQ("Failed to process json message with error: INVALID_ARGUMENT:Unexpected token.\nnon json string\n^", e.what());
    }
}

TEST_F(TestSerialiseUpdateSettings, emptyJson)
{
    EXPECT_NO_THROW(SerialiseUpdateSettings::fromJsonSettings("{}"));
}

TEST_F(TestSerialiseUpdateSettings, preserveJWT)
{
    UpdateSettings before;
    before.setJWToken("TOKEN");
    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);
    EXPECT_EQ(before.getJWToken(), after.getJWToken());
}

TEST_F(TestSerialiseUpdateSettings, preserveTenant)
{
    UpdateSettings before;
    before.setTenantId("TOKEN");
    ASSERT_EQ(before.getTenantId(), "TOKEN");
    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);
    EXPECT_EQ(before.getTenantId(), after.getTenantId());
}

TEST_F(TestSerialiseUpdateSettings, preserveDevice)
{
    UpdateSettings before;
    before.setDeviceId("TOKEN");
    ASSERT_EQ(before.getDeviceId(), "TOKEN");
    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);
    EXPECT_EQ(before.getDeviceId(), after.getDeviceId());
}
