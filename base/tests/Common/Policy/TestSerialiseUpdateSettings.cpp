// Copyright 2023 Sophos Limited. All rights reserved.

#include "TestUpdateSettingsBase.h"

#include "Common/Policy/SerialiseUpdateSettings.h"

#include "tests/Common/UtilityImpl/TestStringGenerator.h"

using namespace Common::Policy;

namespace
{
    const std::string VALID_JSON = R"({
           "sophosCdnURLs": [
           "https://sophosupdate.sophos.com/latest/warehouse"
           ],
            "sophosSusURL": "https://sus.sophosupd.com",
           "updateCache": [
           "https://cache.sophos.com/latest/warehouse"
           ],
           "credential": {"username": "administrator","password": "password"},
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
           "primarySubscription": {"rigidName" : "BaseProduct-RigidName","baseVersion" : "9","tag" : "RECOMMENDED","fixedVersion" : ""},
            "products": [
            {
            "rigidName" : "PrefixOfProduct-SimulateProductA",
            "baseVersion" : "9",
            "tag" : "RECOMMENDED",
            "fixedVersion" : ""
            },
            ],
            "features": ["CORE", "MDR"],
           "installArguments": ["--install-dir","/opt/sophos-av"]
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
                if (pos != std::string::npos)
                {
                    jsonString.replace(pos, oldPartString.size(), newPartString);
                }
            }
            return jsonString;
        }

        static std::string mutateJson(const std::string& oldPartString)
        {
            return mutateJson(oldPartString, "");
        }
    };
}

TEST_F(TestSerialiseUpdateSettings, invalidJsonString)
{
    try
    {
        SerialiseUpdateSettings::fromJsonSettings("non json string");
        FAIL();
    }
    catch (const PolicyParseException& e)
    {
#ifdef SPL_BAZEL
        EXPECT_THAT(e.what(), StartsWith("Failed to process json message with error: INVALID_ARGUMENT: invalid JSON"));
#else
        EXPECT_STREQ("Failed to process json message with error: INVALID_ARGUMENT:Unexpected token.\nnon json string\n^", e.what());
#endif
    }
}

TEST_F(TestSerialiseUpdateSettings, emptyJson)
{
    EXPECT_NO_THROW(SerialiseUpdateSettings::fromJsonSettings("{}"));
}

TEST_F(TestSerialiseUpdateSettings, missingProxyProducesSettingsWithNoProxy)
{
    auto settings = SerialiseUpdateSettings::fromJsonSettings("{}");
    auto proxy = settings.getPolicyProxy();
    EXPECT_TRUE(proxy.empty());
}

TEST_F(TestSerialiseUpdateSettings, validJsonIsValid)
{
    setupFileSystemAndGetMock();
    auto settings = SerialiseUpdateSettings::fromJsonSettings(getValidJson());
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, noUpdateCacheShouldBeValid)
{
    setupFileSystemAndGetMock();
    auto settings =
        SerialiseUpdateSettings::fromJsonSettings(
            mutateJson(R"("https://cache.sophos.com/latest/warehouse")"));
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, emptyUpdateCacheValueShouldBeInvalid)
{
    setupFileSystemAndGetMock();
    auto settings =
        SerialiseUpdateSettings::fromJsonSettings(
            mutateJson(R"(https://cache.sophos.com/latest/warehouse)"));
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, emptySophosUrlValueShouldFailValidation)
{
    setupFileSystemAndGetMock();
    auto settings =
        SerialiseUpdateSettings::fromJsonSettings(
            mutateJson("https://sophosupdate.sophos.com/latest/warehouse"));

    EXPECT_FALSE(settings.verifySettingsAreValid());
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

TEST_F(TestSerialiseUpdateSettings, preserveDelta)
{
    UpdateSettings before;
    before.setUseSdds3DeltaV2(true);
    ASSERT_TRUE(before.getUseSdds3DeltaV2());
    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);
    EXPECT_EQ(before.getUseSdds3DeltaV2(), after.getUseSdds3DeltaV2());
}

TEST_F(TestSerialiseUpdateSettings, defaultValues)
{
    UpdateSettings before;
    ASSERT_FALSE(before.getUseSdds3DeltaV2());
    ASSERT_FALSE(before.getDoForcedUpdate());
    ASSERT_FALSE(before.getDoPausedForcedUpdate());
}

TEST_F(TestSerialiseUpdateSettings, preserveEsmVersion)
{
    UpdateSettings before;
    ESMVersion esmVersion("ESMName", "ESMToken");
    before.setEsmVersion(std::move(esmVersion));
    ASSERT_EQ(before.getEsmVersion().name(), esmVersion.name());
    ASSERT_EQ(before.getEsmVersion().token(), esmVersion.token());

    auto serialised = SerialiseUpdateSettings::toJsonSettings(before);
    auto after = SerialiseUpdateSettings::fromJsonSettings(serialised);

    EXPECT_EQ(before.getEsmVersion().name(), after.getEsmVersion().name());
    EXPECT_EQ(before.getEsmVersion().token(), after.getEsmVersion().token());
}

TEST_F(TestSerialiseUpdateSettings, mssingPrimarySubscriptionShouldFailValidation)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("primarySubscription": {"rigidName" : "BaseProduct-RigidName","baseVersion" : "9","tag" : "RECOMMENDED","fixedVersion" : ""},)";

    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(
        mutateJson(oldString));

    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, mssingCdnUrlsShouldFailValidation)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("sophosCdnURLs")";

    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(
        mutateJson(oldString,"sophos"));

    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, missingSusUrlShouldFailValidation)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("sophosSusURL": "https://sus.sophosupd.com",)";

    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(
        mutateJson(oldString));

    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, missingRigidNameShouldFailValidation)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("rigidName" : "PrefixOfProduct-SimulateProductA")";
    std::string newString = R"("rigidName" : "")";

    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(mutateJson(oldString, newString));

    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, missingTagAndFixedVersionShouldFailValidation)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("primarySubscription": {"rigidName" : "BaseProduct-RigidName","baseVersion" : "9","tag" : "RECOMMENDED","fixedVersion" : ""},)";
    std::string newString = R"("primarySubscription": {"rigidName" : "BaseProduct-RigidName","baseVersion" : "9","tag" : "","fixedVersion" : ""},)";

    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(mutateJson(oldString, newString));

    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, emptyInstallArgumentsShouldFailValidation)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("--install-dir","/opt/sophos-av")";
    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(
        mutateJson(oldString));
    EXPECT_TRUE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, missingInstallArgumentsShouldFailValidation)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("installArguments": ["--install-dir","/opt/sophos-av"])";
    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(mutateJson(oldString));
    EXPECT_TRUE(updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, validJsonStringWithEmptyValueInInstallArgumentsShouldFailValidation)
{
    setupFileSystemAndGetMock();
    std::string oldString = "--install-dir";
    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(mutateJson(oldString));
    EXPECT_FALSE(updateSettings.verifySettingsAreValid());
}


//ESM fault injection
namespace ESMFaultInjection
{
    static std::string getJsonWithESM(const std::string& esmVersion)
    {
        auto json = R"({
                    "sophosCdnURLs": [
                                       "http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid"
                    ],
                    "sophosSusURL": "https://sus.sophosupd.com",
                    "proxy": {
                      "url": "noproxy:",
                      "credential": {
                          "username": "",
                          "password": ""
                      }
                    },
                    "JWToken": "token",
                    "tenantId": "tenantid",
                    "deviceId": "deviceid",
                    "primarySubscription": {
                      "rigidName": "ServerProtectionLinux-Base-component",
                      "baseVersion": "",
                      "tag": "RECOMMMENDED",
                      "fixedVersion": ""
                    },
                    "features": [
                                   "CORE"
                    ],)"
                    + esmVersion +
                    R"(})";

        return json;
    }
}

class TestSerialiseUpdateSettingsParameterized
    :  public ::testing::TestWithParam<std::pair<std::string, bool>>
{
    public:
        void SetUp() override
        {
            loggingSetup_ = Common::Logging::LOGOFFFORTEST();
            mockFileSystem_ = std::make_unique<NaggyMock<MockFileSystem>>();
            ON_CALL(*mockFileSystem_, isDirectory(_)).WillByDefault(Return(true));
        }
        std::unique_ptr<MockFileSystem> mockFileSystem_;
        Common::Logging::ConsoleLoggingSetup loggingSetup_;
};


INSTANTIATE_TEST_SUITE_P(
    TestSerialiseUpdateSettings,
    TestSerialiseUpdateSettingsParameterized,
    ::testing::Values(
        //One is missing
        std::make_pair(R"("esmVersion": { "token": "token" })", false),
        std::make_pair(R"("esmVersion": { "name": "name" })", false),
        //One is empty
        std::make_pair(R"("esmVersion": { "token": "token", "name": "" })", false),
        std::make_pair(R"("esmVersion": { "token": "", "name": "name" })", false),
        //Duplicate
        std::make_pair(R"("esmVersion": { "token": "token", "token": "token2", "name": "name" })", true),
        //Both are empty
        std::make_pair(R"("esmVersion": { "token": "", "name": "" })", true),
        //Both are missing
        std::make_pair(R"("esmVersion": {})", true),
        //Large Strings
        std::make_pair(R"("esmVersion": { "token": ")" + std::string(30000, 't') + R"(", "name": "name" })", true),
        std::make_pair(R"("esmVersion": { "token": "token", "name": ")" + std::string(30000, 't') + R"(" })", true)));

TEST_P(TestSerialiseUpdateSettingsParameterized, esmVersionValidity)
{
    Tests::ScopedReplaceFileSystem fsReplacer;
    fsReplacer.replace(std::move(mockFileSystem_));

    auto [esmVersion, isValid] = GetParam();
    auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(ESMFaultInjection::getJsonWithESM(esmVersion));
    EXPECT_EQ(isValid, updateSettings.verifySettingsAreValid());
}

TEST_F(TestSerialiseUpdateSettings, nonUtf8EsmInputThrows)
{
    setupFileSystemAndGetMock();
    auto nonUtf8Str = generateNonUTF8String();

    try
    {
        auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(ESMFaultInjection::getJsonWithESM(nonUtf8Str));
        FAIL() <<  "didnt throw due to non-utf8";
    }
    catch (const std::invalid_argument& ex)
    {
        EXPECT_STREQ(ex.what(), "Not a valid UTF-8 string");
    }
}

TEST_F(TestSerialiseUpdateSettings, xmlInESMField)
{
    setupFileSystemAndGetMock();
    std::string esmVersion(R"("esmVersion": <?xml version="1.0"?> <token>token</token>})");

    try
    {
        auto updateSettings = SerialiseUpdateSettings::fromJsonSettings(ESMFaultInjection::getJsonWithESM(esmVersion));
        FAIL() <<  "Didnt throw due to xml in field";
    }
    catch (const UpdatePolicySerialisationException& ex)
    {
#ifdef SPL_BAZEL
        EXPECT_THAT(ex.what(), StartsWith("Failed to process json message with error: INVALID_ARGUMENT: invalid JSON"));
#else
        EXPECT_STREQ(ex.what(), "Failed to process json message with error: INVALID_ARGUMENT:Expected a value.\n<?xml version=\"1.0\"?\n^");
#endif
    }
}