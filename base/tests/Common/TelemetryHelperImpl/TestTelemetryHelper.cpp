// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/TelemetryHelperImpl/TelemetryFieldStructure.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/TelemetryHelperImpl/TelemetryObject.h"
#include "Common/TelemetryHelperImpl/TelemetrySerialiser.h"

#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <thread>


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

class ScopeInsertFSMock{
    Common::Telemetry::TelemetryHelper & m_helper; 
    public:
    explicit ScopeInsertFSMock( IFileSystem * mock, Common::Telemetry::TelemetryHelper & helper):
    m_helper(helper)
    {
        m_helper.replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem>(mock)); 
    }
    ~ScopeInsertFSMock()
    {
        m_helper.replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem>(new Common::FileSystem::FileSystemImpl())); 
    }
};


class TestTelemetryHelper : public MemoryAppenderUsingTests
{
public:
    TestTelemetryHelper() : MemoryAppenderUsingTests("TelemetryHelperImpl") {}
};

TEST_F(TestTelemetryHelper, getInstanceReturnsSingleton)
{
    TelemetryHelper& helper1 = TelemetryHelper::getInstance();
    TelemetryHelper& helper2 = TelemetryHelper::getInstance();
    ASSERT_EQ(&helper1, &helper2);
}

TEST_F(TestTelemetryHelper, constructionCreatesDifferentInstance)
{
    TelemetryHelper& helper1 = TelemetryHelper::getInstance();
    TelemetryHelper helper2;
    TelemetryHelper helper3;
    ASSERT_NE(&helper1, &helper2);
    ASSERT_NE(&helper2, &helper3);
    ASSERT_NE(&helper1, &helper3);
}

TEST_F(TestTelemetryHelper, addStringTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", std::string("Ubuntu"));
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addCStringTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", "Ubuntu");
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addIntTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("An int", 3L);
    ASSERT_EQ(R"({"An int":3})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addUnsignedIntTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("uint", 1UL);
    ASSERT_EQ(R"({"uint":1})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addBoolTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("true", true);
    ASSERT_EQ(R"({"true":true})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendInt)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 1L);
    ASSERT_EQ(R"({"array":[1]})", helper.serialise());
    helper.appendValue("array", 2L);
    ASSERT_EQ(R"({"array":[1,2]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendUnsignedInt)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 2UL);
    ASSERT_EQ(R"({"array":[2]})", helper.serialise());
    helper.appendValue("array", 3UL);
    ASSERT_EQ(R"({"array":[2,3]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendString)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", std::string("string"));
    ASSERT_EQ(R"({"array":["string"]})", helper.serialise());
    helper.appendValue("array", std::string("string2"));
    ASSERT_EQ(R"({"array":["string","string2"]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendCString)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", "cstring");
    ASSERT_EQ(R"({"array":["cstring"]})", helper.serialise());
    helper.appendValue("array", "cstring2");
    ASSERT_EQ(R"({"array":["cstring","cstring2"]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendBool)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", true);
    ASSERT_EQ(R"({"array":[true]})", helper.serialise());
    helper.appendValue("array", false);
    ASSERT_EQ(R"({"array":[true,false]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    TelemetryObject telemetryObject;
    helper.appendObject("array", telemetryObject);
    ASSERT_EQ(R"({"array":[{}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendCstringObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", "value1");
    ASSERT_EQ(R"({"array":[{"key1":"value1"}]})", helper.serialise());
    helper.appendObject("array", "key2", "value2");
    ASSERT_EQ(R"({"array":[{"key1":"value1"},{"key2":"value2"}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendIntObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", 1L);
    ASSERT_EQ(R"({"array":[{"key1":1}]})", helper.serialise());
    helper.appendObject("array", "key2", 2L);
    ASSERT_EQ(R"({"array":[{"key1":1},{"key2":2}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendDoubleObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", 1.0);
    ASSERT_EQ(R"({"array":[{"key1":1.0}]})", helper.serialise());
    helper.appendObject("array", "key2", 2.0);
    ASSERT_EQ(R"({"array":[{"key1":1.0},{"key2":2.0}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendStringObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", std::string("value1"));
    ASSERT_EQ(R"({"array":[{"key1":"value1"}]})", helper.serialise());
    helper.appendObject("array", "key2", std::string("value2"));
    ASSERT_EQ(R"({"array":[{"key1":"value1"},{"key2":"value2"}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendUnsignedIntObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", 1UL);
    ASSERT_EQ(R"({"array":[{"key1":1}]})", helper.serialise());
    helper.appendObject("array", "key2", 2UL);
    ASSERT_EQ(R"({"array":[{"key1":1},{"key2":2}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendBoolObject)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendObject("array", "key1", false);
    ASSERT_EQ(R"({"array":[{"key1":false}]})", helper.serialise());
    helper.appendObject("array", "key2", true);
    ASSERT_EQ(R"({"array":[{"key1":false},{"key2":true}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, appendMixed)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendValue("array", 1L);
    helper.appendValue("array", 3UL);
    helper.appendValue("array", false);
    helper.appendValue("array", "cstring");
    helper.appendValue("array", std::string("string"));
    helper.appendObject("array", "obj", std::string("val"));
    ASSERT_EQ(R"({"array":[1,3,false,"cstring","string",{"obj":"val"}]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, incCounter)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1L);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1L);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
    helper.increment("counter", 5L);
    ASSERT_EQ(R"({"counter":7})", helper.serialise());
    helper.increment("counter", -2L);
    ASSERT_EQ(R"({"counter":5})", helper.serialise());
}

TEST_F(TestTelemetryHelper, incCounterByUnsignedInt)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1L);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1UL);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
}

TEST_F(TestTelemetryHelper, incUnsignedIntCounterByUnsignedInt)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", 1UL);
    ASSERT_EQ(R"({"counter":1})", helper.serialise());
    helper.increment("counter", 1UL);
    ASSERT_EQ(R"({"counter":2})", helper.serialise());
}

TEST_F(TestTelemetryHelper, incNegativeCounter)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("counter", -10L);
    ASSERT_EQ(R"({"counter":-10})", helper.serialise());
    helper.increment("counter", 1L);
    ASSERT_EQ(R"({"counter":-9})", helper.serialise());
}

TEST_F(TestTelemetryHelper, incNonExistantValue)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.increment("counter", 3L);
    ASSERT_EQ(R"({"counter":3})", helper.serialise());
}

TEST_F(TestTelemetryHelper, nestedTelem)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("1", 1L);
    helper.set("2", 2UL);
    helper.set("a.nested.string", "string1");
    helper.appendValue("a.nested.array", "string2");
    helper.appendValue("a.nested.array", 1L);
    helper.appendValue("a.nested.array", false);

    ASSERT_EQ(R"({"1":1,"2":2,"a":{"nested":{"array":["string2",1,false],"string":"string1"}}})", helper.serialise());
}

TEST_F(TestTelemetryHelper, registerResetCallback)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)));
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));           
}

TEST_F(TestTelemetryHelper, reregisterResetCallback)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)));
    ASSERT_THROW(
        helper.registerResetCallback(dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)),
        std::logic_error);
    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));
}

TEST_F(TestTelemetryHelper, unregisterResetCallback)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)));
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));           
    helper.reset();
    ASSERT_FALSE(dummy.hasCallbackBeenCalled());
}

TEST_F(TestTelemetryHelper, registerResetCallbackGetsCalled)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy)));
    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));
}

TEST_F(TestTelemetryHelper, multipleRegisterResetCallbackGetsCalled)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy1("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy1.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy1)));

    DummyTelemetryProvider dummy2("dummy2");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy2.getCookie(), std::bind(&DummyTelemetryProvider::callback, &dummy2)));

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy1.getCookie()));
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy2.getCookie()));
}

TEST_F(TestTelemetryHelper, registerResetCallbackGetsCalledMultipleTimes)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(),
        std::bind(&DummyTelemetryProvider::callback, &dummy)));

    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());

    dummy.resetTestState();

    helper.reset();
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));
}

TEST_F(TestTelemetryHelper, multipleRegisterResetCallbackGetsCalledMultipleTimes)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy1("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy1.getCookie(),
        std::bind(&DummyTelemetryProvider::callback, &dummy1)));

    DummyTelemetryProvider dummy2("dummy2");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy2.getCookie(),
        std::bind(&DummyTelemetryProvider::callback, &dummy2)));

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());
    dummy1.resetTestState();
    dummy2.resetTestState();

    helper.reset();
    ASSERT_TRUE(dummy1.hasCallbackBeenCalled());
    ASSERT_TRUE(dummy2.hasCallbackBeenCalled());

    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy1.getCookie()));
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy2.getCookie()));
}

TEST_F(TestTelemetryHelper, mergeJsonInWithExistingData)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("OS Name", std::string("Ubuntu"));
    ASSERT_EQ(R"({"OS Name":"Ubuntu"})", helper.serialise());
    std::string json = R"({"counter":4})";
    helper.mergeJsonIn("merged", json);
    ASSERT_EQ(R"({"OS Name":"Ubuntu","merged":{"counter":4}})", helper.serialise());
}

TEST_F(TestTelemetryHelper, mergeJsonInWithoutExistingData)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string json = R"({"counter":4})";
    helper.mergeJsonIn("merged", json);
    ASSERT_EQ(R"({"merged":{"counter":4}})", helper.serialise());
}

TEST_F(TestTelemetryHelper, mergeJsonInTwice)
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

TEST_F(TestTelemetryHelper, mergeInvalidJsonIn)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string json = R"({"counter":})";
    ASSERT_THROW(helper.mergeJsonIn("merged", json), nlohmann::detail::parse_error);
}

void appendLots(const std::string& arrayName, int numberToAdd)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    for (long i = 0; i < numberToAdd; ++i)
    {
        helper.appendValue(arrayName, i);
        usleep(1);
    }
}

TEST_F(TestTelemetryHelper, resetAndSerialiseExecutesCallbacksAndReturnsJson)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    DummyTelemetryProvider dummy("dummy1");
    ASSERT_NO_THROW(helper.registerResetCallback(
        dummy.getCookie(),
        std::bind(&DummyTelemetryProvider::callback, &dummy)));

    helper.set("a", "b");
    std::string json = helper.serialiseAndReset();
    ASSERT_EQ(R"({"a":"b"})", json);
    ASSERT_TRUE(dummy.hasCallbackBeenCalled());
    ASSERT_NO_THROW(helper.unregisterResetCallback(dummy.getCookie()));
}

TEST_F(TestTelemetryHelper, dataNotLostDuringMultiThreadedUse)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();

    const int numberOfItemsInArray = 300;
    const std::string arrayName = "array";

    std::thread t1(appendLots, arrayName, numberOfItemsInArray);
    usleep(100);
    std::string part1 = helper.serialiseAndReset();
    usleep(50);
    std::string part2 = helper.serialiseAndReset();
    usleep(10);
    t1.join();
    std::string part3 = helper.serialiseAndReset();

    std::cout << part1 << std::endl;
    std::cout << part2 << std::endl;
    std::cout << part3 << std::endl;

    TelemetryObject part1Obj = TelemetrySerialiser::deserialise(part1);
    TelemetryObject part2Obj = TelemetrySerialiser::deserialise(part2);
    TelemetryObject part3Obj = TelemetrySerialiser::deserialise(part3);

    int array1size = 0;
    int array2size = 0;
    int array3size = 0;

    try
    {
        array1size = part1Obj.getChildObjects()[arrayName].getArray().size();
    }
    catch (...)
    {
        // doesn't matter
    }

    try
    {
        array2size = part2Obj.getChildObjects()[arrayName].getArray().size();
    }
    catch (...)
    {
        // doesn't matter
    }

    try
    {
        array3size = part3Obj.getChildObjects()[arrayName].getArray().size();
    }
    catch (...)
    {
        // doesn't matter
    }

    ASSERT_EQ(numberOfItemsInArray, array1size + array2size + array3size);
}

TEST_F(TestTelemetryHelper, telemtryStatSerialisedCorrectly)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendStat("statName", 1);
    helper.appendStat("statName", 6);
    helper.appendStat("statName", 10);
    helper.updateTelemetryWithStats();

    ASSERT_EQ(R"({"statName-avg":5.666666666666667,"statName-max":10.0,"statName-min":1.0})", helper.serialise());
}

TEST_F(TestTelemetryHelper, telemtryStatStdDeviationSerialisedCorrectly)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.appendStat("statName", 1);
    helper.appendStat("statName", 6);
    helper.appendStat("statName", 10);
    helper.updateTelemetryWithAllStdDeviationStats();

    EXPECT_THAT(helper.serialise(),::testing::HasSubstr("\"statName-std-deviation\":3.68178"));
}

TEST_F(TestTelemetryHelper, TelemetryAndStatsAreSavedCorrectly)
{
    MockFileSystem* mockFileSystemPtr  = new StrictMock<MockFileSystem>;
    EXPECT_CALL(*mockFileSystemPtr, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystemPtr, writeFileAtomically(_,
            R"({"rootkey":{"a":"b"},"statskey":{"statName":[1.0,6.0,10.0]}})", _));

    TelemetryHelper& helper = TelemetryHelper::getInstance();
    ScopeInsertFSMock s(mockFileSystemPtr, helper);
    helper.reset();
    helper.appendStat("statName", 1);
    helper.appendStat("statName", 6);
    helper.appendStat("statName", 10);

    helper.set("a", "b");

    helper.save();
}

TEST_F(TestTelemetryHelper, updateStatsCollectionFromSavedTelemetry)
{
    MockFileSystem* mockFileSystemPtr  = new StrictMock<MockFileSystem>;
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    ScopeInsertFSMock s(mockFileSystemPtr, helper);

    helper.reset();

    std::string telemetryFilePath{"/opt/sophos-spl/base/telemetry/cache/test-telemetry.json"};
    EXPECT_CALL(*mockFileSystemPtr, isFile(telemetryFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystemPtr, readFile(telemetryFilePath, 1000000)).WillOnce(Return(R"({"rootkey":{"a":"b"},"statskey":{"statName":[1.0,6.0,10.0]}})"));
    EXPECT_CALL(*mockFileSystemPtr, removeFile(telemetryFilePath));

    helper.restore("test");

    helper.updateTelemetryWithStats();

    ASSERT_EQ(R"({"a":"b","statName-avg":5.666666666666667,"statName-max":10.0,"statName-min":1.0})", helper.serialise());
}

TEST_F(TestTelemetryHelper, updateStatsFromSavedTelemetryInvalidJson)
{
    Common::Logging::ConsoleLoggingSetup loggingSetup;

    testing::internal::CaptureStderr();

    MockFileSystem* mockFileSystemPtr  = new StrictMock<MockFileSystem>;

    TelemetryHelper& helper = TelemetryHelper::getInstance();
    ScopeInsertFSMock s(mockFileSystemPtr, helper);

    helper.reset();
    std::string telemetryFilePath{"/opt/sophos-spl/base/telemetry/cache/test-telemetry.json"};
    EXPECT_CALL(*mockFileSystemPtr, isFile(telemetryFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystemPtr, readFile(telemetryFilePath, 1000000)).WillOnce(Return(R"({"rootkey":{"a":"b"},"statskey":{"statName";[1.0,6.0,10.0]}})"));
    EXPECT_CALL(*mockFileSystemPtr, removeFile(telemetryFilePath));

    helper.restore("test");

    helper.updateTelemetryWithStats();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Restore Telemetry unsuccessful reason: "));
    ASSERT_EQ(R"({})", helper.serialise());
}

TEST_F(TestTelemetryHelper, addValueToSet)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    std::string aString = "string";
    helper.addValueToSet("my-set","a");
    helper.addValueToSet("my-set","b");
    helper.addValueToSet("my-set","b");
    helper.addValueToSet("my-set","c");
    helper.addValueToSet("my-set","a");
    helper.addValueToSet("my-set",aString);
    helper.addValueToSet("my-set",aString);
    helper.addValueToSet("my-set",false);
    helper.addValueToSet("my-set",false);
    helper.addValueToSet("my-set",1.0213);
    helper.addValueToSet("my-set",1.0);
    helper.addValueToSet("my-set",1UL);
    helper.addValueToSet("my-set",2L);
    helper.addValueToSet("my-set",2L);
    helper.addValueToSet("my-set",2UL);
    helper.addValueToSet("my-set",2UL);
    ASSERT_EQ(R"({"my-set":["a","b","c","string",false,1.0213,1.0,1,2]})", helper.serialise());
}

TEST_F(TestTelemetryHelper, restructureTestMultipleEntries)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("updatescheduler.esmName", std::string("esmName"));
    helper.set("updatescheduler.something", std::string("something"));
    ASSERT_EQ(R"({"updatescheduler":{"esmName":"esmName","something":"something"}})", helper.serialise());
    helper.restructureTelemetry();
    EXPECT_EQ(R"({"esmName":"esmName","updatescheduler":{"something":"something"}})", helper.serialise());
}

TEST_F(TestTelemetryHelper, restructureTestSingleEntry)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("updatescheduler.esmName", std::string("esmName"));
    helper.restructureTelemetry();
    EXPECT_EQ(R"({"esmName":"esmName"})", helper.serialise());
}

TEST_F(TestTelemetryHelper, restructureEmptyTelemetry)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.restructureTelemetry();
    EXPECT_EQ(R"({})", helper.serialise());
}

TEST_F(TestTelemetryHelper, restructureEntryDoesntExist)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    helper.set("updatescheduler.download-state", 0L);
    helper.set("updatescheduler.subscriptions-ServerProtectionLinux-Base", "RECOMMENDED");
    helper.restructureTelemetry();
    EXPECT_EQ(R"({"updatescheduler":{"download-state":0,"subscriptions-ServerProtectionLinux-Base":"RECOMMENDED"}})", helper.serialise());
}

TEST_F(TestTelemetryHelper, restructureAllExpectedEntries)
{
    TelemetryHelper& helper = TelemetryHelper::getInstance();
    helper.reset();
    //For test purposes treat the second field as a value
    for (const auto& [interestingField, value] : fieldsToMoveToTopLevel)
    {
        helper.set(interestingField, value);
    }

    std::string expectedFirst = R"({"base-telemetry":{"deviceId":"deviceId","tenantId":"tenantId"},"updatescheduler":{"esmName":"esmName","esmToken":"esmToken","suite-version":"suiteVersion"}})";
    ASSERT_EQ(expectedFirst, helper.serialise());

    helper.restructureTelemetry();
    EXPECT_EQ(R"({"deviceId":"deviceId","esmName":"esmName","esmToken":"esmToken","suiteVersion":"suiteVersion","tenantId":"tenantId"})", helper.serialise());
}