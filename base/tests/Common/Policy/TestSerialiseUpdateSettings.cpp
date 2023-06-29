// Copyright 2023 Sophos All rights reserved.

#include "Common/Policy/SerialiseUpdateSettings.h"

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

TEST_F(TestSerialiseUpdateSettings, preserveJWT)
{
    UpdateSettings before;
    before.setJWToken("TOKEN");
    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);
    EXPECT_EQ(before.getJWToken(), after.getJWToken());
}
