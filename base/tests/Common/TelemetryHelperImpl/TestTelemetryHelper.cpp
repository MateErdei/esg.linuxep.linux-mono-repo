///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetryObject.h>
#include <include/gtest/gtest.h>
#include <json.hpp>

using namespace Common::Telemetry;

class DummyTelemetryProvider
{
public:
    explicit DummyTelemetryProvider(const std::string& cookie) : m_cookie(cookie), m_callbackCalled(false) {}

    void callback() { m_callbackCalled = true; }

    bool hasCallbackBeenCalled() { return m_callbackCalled; }

    void resetTestState() { m_callbackCalled = false; }

    std::string getCookie() { return m_cookie; }

private:
    std::string m_cookie;
    bool m_callbackCalled = false;
};

TEST(TestTelemetryHelper, getInstanceReturnsSingleton) // NOLINT
{
    TelemetryHelper& helper1 = TelemetryHelper::getInstance();
    TelemetryHelper& helper2 = TelemetryHelper::getInstance();
    ASSERT_EQ(&helper1, &helper2);
}

TEST(TestTelemetryHelper, constructionCreatesDifferentInstance) // NOLINT
{
    TelemetryHelper& helper1 = TelemetryHelper::getInstance();
    TelemetryHelper helper2;
    TelemetryHelper helper3;
    ASSERT_NE(&helper1, &helper2);
    ASSERT_NE(&helper2, &helper3);
    ASSERT_NE(&helper1, &helper3);
}

TEST(TestTelemetryHelper, addStringTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", std::string("Ubuntu"));
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST(TestTelemetryHelper, addCStringTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", "Ubuntu");
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST(TestTelemetryHelper, addIntTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("An int", 3);
    ASSERT_EQ(R"({"An int":3})", helper.serialise());
}

TEST(TestTelemetryHelper, addUnsignedIntTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("uint", 1U);
    ASSERT_EQ(R"({"uint":1})", helper.serialise());
}

TEST(TestTelemetryHelper, addBoolTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("true", true);
    ASSERT_EQ(R"({"true":true})", helper.serialise());
}

TEST(TestTelemetryHelper, appendInt) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 1);
    ASSERT_EQ(R"({"array":[1]})", helper.serialise());
    helper.appendValue("array", 2);
    ASSERT_EQ(R"({"array":[1,2]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendUnsignedInt) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 2u);
    ASSERT_EQ(R"({"array":[2]})", helper.serialise());
    helper.appendValue("array", 3u);
    ASSERT_EQ(R"({"array":[2,3]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendString) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", std::string("string"));
    ASSERT_EQ(R"({"array":["string"]})", helper.serialise());
    helper.appendValue("array", std::string("string2"));
    ASSERT_EQ(R"({"array":["string","string2"]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendCString) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", "cstring");
    ASSERT_EQ(R"({"array":["cstring"]})", helper.serialise());
    helper.appendValue("array", "cstring2");
    ASSERT_EQ(R"({"array":["cstring","cstring2"]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendBool) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", true);
    ASSERT_EQ(R"({"array":[true]})", helper.serialise());
    helper.appendValue("array", false);
    ASSERT_EQ(R"({"array":[true,false]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array");
    ASSERT_EQ(R"({"array":[{}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendCstringObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", "value1");
    ASSERT_EQ(R"({"array":[{"key1":"value1"}]})", helper.serialise());
    helper.appendObject("array", "key2", "value2");
    ASSERT_EQ(R"({"array":[{"key1":"value1"},{"key2":"value2"}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendIntObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", 1);
    ASSERT_EQ(R"({"array":[{"key1":1}]})", helper.serialise());
    helper.appendObject("array", "key2", 2);
    ASSERT_EQ(R"({"array":[{"key1":1},{"key2":2}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendStringObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", std::string("value1"));
    ASSERT_EQ(R"({"array":[{"key1":"value1"}]})", helper.serialise());
    helper.appendObject("array", "key2", std::string("value2"));
    ASSERT_EQ(R"({"array":[{"key1":"value1"},{"key2":"value2"}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendUnsignedIntObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", 1u);
    ASSERT_EQ(R"({"array":[{"key1":1}]})", helper.serialise());
    helper.appendObject("array", "key2", 2u);
    ASSERT_EQ(R"({"array":[{"key1":1},{"key2":2}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendBoolObject) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", false);
    ASSERT_EQ(R"({"array":[{"key1":false}]})", helper.serialise());
    helper.appendObject("array", "key2", true);
    ASSERT_EQ(R"({"array":[{"key1":false},{"key2":true}]})", helper.serialise());
}

TEST(TestTelemetryHelper, appendMixed) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 1);
    helper.appendValue("array", 3u);
    helper.appendValue("array", false);
    helper.appendValue("array", "cstring");
    helper.appendValue("array", std::string("string"));
    helper.appendObject("array", "obj", std::string("val"));
    ASSERT_EQ(R"({"array":[1,3,false,"cstring","string",{"obj":"val"}]})", helper.serialise());
}

TEST(TestTelemetryHelper, incCounter) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
    helper.increment("counter", 5);
    ASSERT_EQ(R"({"counter":7})", helper.serialise());
    helper.increment("counter", -2);
    ASSERT_EQ(R"({"counter":5})", helper.serialise());
}

TEST(TestTelemetryHelper, incCounterByUnsignedInt) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1u);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
}

TEST(TestTelemetryHelper, incUnsignedIntCounterByUnsignedInt) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1u);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1u);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
}

TEST(TestTelemetryHelper, incNegativeCounter) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", -10);
    ASSERT_EQ(R"({"counter":-10})", helper.serialise());
    helper.increment("counter", 1);
    ASSERT_EQ(R"({"counter":-9})", helper.serialise());
}

TEST(TestTelemetryHelper, incNonExistantValue) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    ASSERT_THROW(helper.increment("counter", 1), std::logic_error); // NOLINT
}

TEST(TestTelemetryHelper, nestedTelem) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("1", 1);
    helper.set("2", 2u);
    helper.set("a.nested.string", "string1");
    helper.appendValue("a.nested.array", "string2");
    helper.appendValue("a.nested.array", 1);
    helper.appendValue("a.nested.array", false);

    ASSERT_EQ(R"({"1":1,"2":2,"a":{"nested":{"array":["string2",1,false],"string":"string1"}}})", helper.serialise());
}

TEST(TestTelemetryHelper, registerResetCallback) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy))); // NOLINT
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));            // NOLINT
}

TEST(TestTelemetryHelper, reregisterResetCallback) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy))); // NOLINT
    ASSERT_THROW(
        helper.registerResetCallback(dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)),
        std::logic_error); // NOLINT
    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie())); // NOLINT
}

TEST(TestTelemetryHelper, unregisterResetCallback) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy))); // NOLINT
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));            // NOLINT
    helper.reset();
    ASSERT_FALSE(dummy.hasCallbackBeenCalled());
}

TEST(TestTelemetryHelper, registerResetCallbackGetsCalled) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy))); // NOLINT
    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie())); // NOLINT
}

TEST(TestTelemetryHelper, multipleRegisterResetCallbackGetsCalled) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy1("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy1.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy1))); // NOLINT

    DummyTelemetryProvider dummy2("dummy2");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy2.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy2))); // NOLINT

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy1.getCookie())); // NOLINT
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy2.getCookie())); // NOLINT
}

TEST(TestTelemetryHelper, registerResetCallbackGetsCalledMultipleTimes) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback( // NOLINT
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)));

    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());

    dummy.resetTestState();

    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie())); // NOLINT
}

TEST(TestTelemetryHelper, multipleRegisterResetCallbackGetsCalledMultipleTimes) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy1("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback( // NOLINT
        dummy1.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy1)));

    DummyTelemetryProvider dummy2("dummy2");
    ASSERT_NO_THROW(helper.registerResetCallback( // NOLINT
        dummy2.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy2)));

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());
    dummy1.resetTestState();
    dummy2.resetTestState();

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy1.getCookie())); // NOLINT
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy2.getCookie())); // NOLINT
}

TEST(TestTelemetryHelper, mergeJsonInWithExistingData) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", std::string("Ubuntu"));
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
    std::string json = R"({"counter":4})";
    helper.mergeJsonIn("merged", json);
    ASSERT_EQ(R"({"OS Name":"Ubuntu","merged":{"counter":4}})", helper.serialise());
}

TEST(TestTelemetryHelper, mergeJsonInWithoutExistingData) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string json = R"({"counter":4})";
    helper.mergeJsonIn("merged", json);
    ASSERT_EQ(R"({"merged":{"counter":4}})", helper.serialise());
}

TEST(TestTelemetryHelper, mergeJsonInTwice) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string json = R"({"counter":4})";
    helper.mergeJsonIn("merged", json);
    std::string jsonMerged = R"({"merged":{"counter":4}})";
    ASSERT_EQ(jsonMerged, helper.serialise());
    helper.mergeJsonIn("merged", json);
    ASSERT_EQ(jsonMerged, helper.serialise());
}

TEST(TestTelemetryHelper, mergeInvalidJsonIn) // NOLINT
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string json = R"({"counter":})";
    ASSERT_THROW(helper.mergeJsonIn("merged", json), nlohmann::detail::parse_error); // NOLINT
}