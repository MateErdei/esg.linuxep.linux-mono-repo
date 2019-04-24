///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryObject.h>

#include <include/gtest/gtest.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

using namespace Common::Telemetry;

class TestTelemetryHelper : public ::testing::Test
{
public:
    TestTelemetryHelper()
    {

    }

};

TEST_F(TestTelemetryHelper, constructor) //NOLINT
{
    TelemetryHelper helper;
}

TEST_F(TestTelemetryHelper, addStringTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("OS Name", std::string("Ubuntu"));
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addCStringTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("OS Name", "Ubuntu");
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addIntTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("An int", 3);
    ASSERT_EQ(R"({"An int":3})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addUnsignedIntTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("uint", 1U);
    ASSERT_EQ(R"({"uint":1})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addBoolTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("true", true);
    ASSERT_EQ(R"({"true":true})", helper.serialise());
}

TEST_F(TestTelemetryHelper, nestedTelem) //NOLINT
{
    TelemetryHelper helper;
    helper.set("1", 1);
    helper.set("2", 2u);
    helper.set("a.nested.string", "string1");
    helper.append("a.nested.array", "string2");
    //helper.append("a.nested.array", 1);
    //helper.append("a.nested.array", false);
    //helper.append("a.nested.array.nestedarray", 3);
    //helper.append("a.nested.array.nestedarray", 4);

    std::cout << helper.serialise() << std::endl;

    //ASSERT_EQ(R"({"true":true})", helper.serialise());
}



