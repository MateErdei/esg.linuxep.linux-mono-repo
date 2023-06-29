// Copyright 2023 Sophos All rights reserved.

#include "Common/Policy/SerialiseUpdateSettings.h"
#include "Policy/PolicyParseException.h"

#include "TestUpdateSettingsBase.h"

namespace
{
    const std::string VALID_JSON = R"({
           "sophosURLs": [
           "https://sophosupdate.sophos.com/latest/warehouse"
           ],
           "updateCache": [
           "https://cache.sophos.com/latest/warehouse"
           ],
           "credential": {
           "username": "administrator",
           "password": "password"
           },
           "proxy": {
           "url": "noproxy:",
           "credential": {
           "username": "",
           "password": "",
           "proxyType": ""
            }
           },
           "manifestNames" : ["manifest.dat"],
           "optionalManifestNames" : ["flags_manifest.dat"],
           "primarySubscription": {
            "rigidName" : "BaseProduct-RigidName",
            "baseVersion" : "9",
            "tag" : "RECOMMENDED",
            "fixedVersion" : ""
            },
            "products": [
            {
            "rigidName" : "PrefixOfProduct-SimulateProductA",
            "baseVersion" : "9",
            "tag" : "RECOMMENDED",
            "fixedVersion" : ""
            },
            ],
            "features": ["CORE", "MDR"],
           "installArguments": [
           "--install-dir",
           "/opt/sophos-av"
           ]
           })";

    class TestSerialiseUpdateSettings : public TestUpdateSettingsBase
    {
    public:
        static std::string getValidJson()
        {
            return VALID_JSON;
        }

        static std::string mutateJson(const std::string& oldPartString, const std::string& newPartString)
        {
            std::string jsonString = VALID_JSON;
            if (!oldPartString.empty())
            {
                size_t pos = jsonString.find(oldPartString);

                EXPECT_TRUE(pos != std::string::npos);

                jsonString.replace(pos, oldPartString.size(), newPartString);
            }
            return jsonString;
        }
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

TEST_F(TestSerialiseUpdateSettings, validJsonIsValid)
{
    setupFileSystemAndGetMock();
    auto settings = SerialiseUpdateSettings::fromJsonSettings(getValidJson());
    EXPECT_TRUE(settings.verifySettingsAreValid());
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
