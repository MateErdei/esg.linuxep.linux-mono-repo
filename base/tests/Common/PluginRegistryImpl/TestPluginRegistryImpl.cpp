/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/PluginRegistryImpl/PluginRegistryException.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>

using namespace ::testing;

class PluginRegistryTests : public ::testing::Test
{
public:

    void SetUp() override
    {

    }
    void TearDown() override
    {

    }

    std::string createJsonString(const std::string & oldPartString, const std::string & newPartString)
    {
        std::string jsonString = R"({
                                    "policyAppIds": [
                                    "app1"
                                     ],
                                     "statusAppIds": [
                                      "app2"
                                     ],
                                     "pluginName": "PluginName",
                                     "xmlTranslatorPath": "path1",
                                     "executableFullPath": "path2",
                                     "executableArguments": [
                                      "arg1"
                                     ],
                                     "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ],
                                     "executableUserAndGroup": "user:group"
                                    })";

        if(!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);

        }

        return jsonString;
    }


    std::string cleanupStringForCompare(std::string &value)
    {
        std::string search("\n");

        // remove new lines
        for (int i = value.find(search); i >= 0; i = value.find(search))
        {
            value.replace(i, search.size(), "");
        }

        search = " ";
        //remove spaces
        for (int i = value.find(search); i >= 0; i = value.find(search))
        {
            value.replace(i, search.size(), "");
        }

        return value;
    }

    Common::PluginRegistryImpl::PluginInfo createDefaultPluginInfo()
    {
        Common::PluginRegistryImpl::PluginInfo pluginInfo;
        pluginInfo.setPluginName("PluginName");
        pluginInfo.setXmlTranslatorPath("path1");
        pluginInfo.setExecutableFullPath("path2");
        pluginInfo.setExecutableArguments({"arg1"});
        pluginInfo.addExecutableEnvironmentVariables( "hello", "world");
        pluginInfo.setPolicyAppIds({"app1"});
        pluginInfo.setStatusAppIds({"app2"});
        pluginInfo.setExecutableUserAndGroup("user:group");

        return std::move(pluginInfo);
    }

    ::testing::AssertionResult pluginInfoSimilar( const char* m_expr,
                                                      const char* n_expr,
                                                      const Common::PluginRegistryImpl::PluginInfo & expected,
                                                      const Common::PluginRegistryImpl::PluginInfo& resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if ( expected.getPluginName() != resulted.getPluginName())
        {
            return ::testing::AssertionFailure() << s.str() << " plugin name differs: \n expected: "
                                                 <<  expected.getPluginName()
                                                 << "\n result: " <<  resulted.getPluginName();
        }

        if( expected.getExecutableFullPath() != resulted.getExecutableFullPath())
        {
            return ::testing::AssertionFailure() << s.str() << " Executable full path differs: \n expected: "
                                                 <<  expected.getExecutableFullPath()
                                                 << "\n result: " <<  resulted.getExecutableFullPath();
        }

        if( expected.getPluginIpcAddress() != resulted.getPluginIpcAddress())
        {
            return ::testing::AssertionFailure() << s.str() << " Plugin Ipc address differs: \n expected: "
                                                 <<  expected.getPluginIpcAddress()
                                                 << "\n result: " <<  resulted.getPluginIpcAddress();
        }

        if( expected.getXmlTranslatorPath() != resulted.getXmlTranslatorPath())
        {
            return ::testing::AssertionFailure() << s.str() << " Xml translator path differs: \n expected: "
                                                 <<  expected.getXmlTranslatorPath()
                                                 << "\n result: " <<  resulted.getXmlTranslatorPath();
        }

        if( expected.getExecutableUserAndGroupAsString() != resulted.getExecutableUserAndGroupAsString())
        {
            return ::testing::AssertionFailure() << s.str() << " User And Group string differs: \n expected: "
                                                 <<  expected.getExecutableUserAndGroupAsString()
                                                 << "\n result: " <<  resulted.getExecutableUserAndGroupAsString();
        }

        {
            std::vector<std::string> expectedValues = expected.getPolicyAppIds();
            std::vector<std::string> resultedValues = resulted.getPolicyAppIds();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            if( expectedValues != resultedValues)
            {
                return ::testing::AssertionFailure() << s.str() << " Policy app ids differs: \n expected: "
                                                        <<  ::testing::PrintToString(expectedValues)
                                                        << "\n result: " <<  ::testing::PrintToString(resultedValues);
            }
        }

        {
            std::vector<std::string> expectedValues = expected.getStatusAppIds();
            std::vector<std::string> resultedValues = resulted.getStatusAppIds();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            if( expectedValues != resultedValues)
            {
                return ::testing::AssertionFailure() << s.str() << " Status app ids differs: \n expected: "
                                                     <<  ::testing::PrintToString(expectedValues)
                                                     << "\n result: " <<  ::testing::PrintToString(resultedValues);
            }

        }

        {
            std::vector<std::string> expectedValues = expected.getExecutableArguments();
            std::vector<std::string> resultedValues = resulted.getExecutableArguments();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            if( expectedValues != resultedValues)
            {
                return ::testing::AssertionFailure() << s.str() << " Executable arguments differs: \n expected: "
                                                        <<  ::testing::PrintToString(expectedValues)
                                                        << "\n result: " <<  ::testing::PrintToString(resultedValues);
            }

        }

        {
            std::vector<std::pair<std::string, std::string>> expectedValues = expected.getExecutableEnvironmentVariables();
            std::vector<std::pair<std::string, std::string>> resultedValues = resulted.getExecutableEnvironmentVariables();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            if( expectedValues != resultedValues)
            {
                return ::testing::AssertionFailure() << s.str() << " Executable environment variables differs: \n expected: "
                                                        <<  ::testing::PrintToString(expectedValues)
                                                        << "\n result: " <<  ::testing::PrintToString(resultedValues);
            }

        }

        return ::testing::AssertionSuccess();
    }

};


// basic tests

TEST_F( PluginRegistryTests, addPolicyAppIdAddsNewValueCorrectly) //NOLINT
{
   Common::PluginRegistryImpl::PluginInfo pluginInfo;

   std::string policyAppId = "AppId1";

    pluginInfo.addPolicyAppIds(policyAppId);

   EXPECT_EQ(pluginInfo.getPolicyAppIds().size(), 1);
   EXPECT_EQ(pluginInfo.getPolicyAppIds()[0], policyAppId);

}


TEST_F( PluginRegistryTests, addStatusAppIdAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string statusAppId = "AppId1";

    pluginInfo.addPolicyAppIds(statusAppId);

    EXPECT_EQ(pluginInfo.getPolicyAppIds().size(), 1);
    EXPECT_EQ(pluginInfo.getPolicyAppIds()[0], statusAppId);
}

TEST_F( PluginRegistryTests, addExecutableArgumentsAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string arg = "Arg1";

    pluginInfo.addExecutableArguments(arg);

    EXPECT_EQ(pluginInfo.getExecutableArguments().size(), 1);
    EXPECT_EQ(pluginInfo.getExecutableArguments()[0], arg);
}

TEST_F( PluginRegistryTests, addExecutableEnvironmentVariablesAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string envName = "envName1";
    std::string envValue = "envValue1";

    pluginInfo.addExecutableEnvironmentVariables(envName, envValue);

    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables().size(), 1);
    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables()[0].first, envName);
    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables()[0].second, envValue);
}

TEST_F( PluginRegistryTests, setExecutableFullPathAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string path = "/tmp/some-path/some-exe";

    pluginInfo.setExecutableFullPath(path);

    EXPECT_EQ(pluginInfo.getExecutableFullPath(), path);
}

TEST_F( PluginRegistryTests, setXmlTranslatorPathAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string path = "/tmp/some-path/some-exe";

    pluginInfo.setXmlTranslatorPath(path);

    EXPECT_EQ(pluginInfo.getXmlTranslatorPath(), path);
}

TEST_F( PluginRegistryTests, setPluginNameAddsNewValueCorrectly) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string name = "pluginName";

    pluginInfo.setPluginName(name);

    EXPECT_EQ(pluginInfo.getPluginName(), name);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoData) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("",""),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsEmptyString) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({""});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("app1",""),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsEmpty) //NOLINT
{
    std::string oldString = R"("app1")";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsMissing) //NOLINT
{
        std::string oldString = R"("policyAppIds": [
                                    "app1"
                                     ],)";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenTwoPolicyAppIds) //NOLINT
{
    std::string oldString = R"("policyAppIds": [
                                    "app1"
                                     ],)";

    std::string newString = R"("policyAppIds": [
                                    "app1",
                                    "app2"
                                     ],)";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.addPolicyAppIds("app2");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsEmptyString) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({""});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("app2",""),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsEmpty) //NOLINT
{
    std::string oldString = R"("app2")";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsMissing) //NOLINT
{
    std::string oldString = R"(                                 "statusAppIds": [
                                      "app2"
                                     ],)";


    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenTwoStatusAppIds) //NOLINT
{
    std::string oldString = R"("app2")";

    std::string newString = R"("app2", "app3")";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.addStatusAppIds("app3");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPluginNameIsEmptyString) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    std::string pluginName = "Anything";

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPluginName(pluginName);

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("PluginName",""),pluginName)
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPluginNameIsMissing) //NOLINT
{
    std::string oldString = R"("pluginName": "PluginName",)";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPluginName("Anything");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString,newString),"Anything")
            );
}


TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenXmlTranslatorPathIsEmptyString) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("path1",""),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenXmlTranslatorPathIsMissing) //NOLINT
{
    std::string oldString = R"( "xmlTranslatorPath": "path1",)";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenExecutableArgsIsEmpty) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableArguments({""});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("arg1",""),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenExecutableArgsIsMissing) //NOLINT
{
    std::string oldString = R"(     "executableArguments": [
                                      "arg1"
                                     ],)";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableArguments({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenTwoExecutableArgs) //NOLINT
{
    std::string oldString = R"(     "executableArguments": [
                                      "arg1"
                                     ],)";

    std::string newString = R"(     "executableArguments": [
                                      "arg1",
                                      "arg2"
                                     ],)";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.addExecutableArguments("arg2");

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenEnvironmentVariablesIsEmpty) //NOLINT
{
    std::string oldString = R"({
                                       "name": "hello",
                                       "value": "world"
                                      })";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    std::vector<std::pair<std::string, std::string>> pairs;
    expectedPluginInfo.setExecutableEnvironmentVariables(pairs);

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString,newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenEnvironmentVariablesIsMissing) //NOLINT
{
    std::string oldString = R"(  "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ],)";

    std::string newString;

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableEnvironmentVariables({});

    EXPECT_PRED_FORMAT2(
            pluginInfoSimilar,
            expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenEnvironmentVariablesIsTwoPairs) //NOLINT
{
    std::string oldString = R"({
                                       "name": "hello",
                                       "value": "world"
                                      })";

    std::string newString = R"({
                               "name": "hello",
                               "value": "world"
                                },
                                {
                               "name": "goodbye",
                               "value": "earth"
                                })";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.addExecutableEnvironmentVariables("goodbye", "earth");

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo,
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString,newString),"PluginName")
            );
}

TEST_F( PluginRegistryTests, pluginInfoSerializeToStringReturnsExpectedData) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    pluginInfo = createDefaultPluginInfo();

    std::string expected = createJsonString("","");
    expected = cleanupStringForCompare(expected);
    std::string result = Common::PluginRegistryImpl::PluginInfo::serializeToString(pluginInfo);
    result = cleanupStringForCompare(result);

    EXPECT_EQ(expected, result);
}

TEST_F( PluginRegistryTests, pluginInfoSerializeToStringWithMultipleEnvironmentVariablesReturnsExpectedData) //NOLINT
{
    std::string oldString = R"({
                                       "name": "hello",
                                       "value": "world"
                                      })";

    std::string newString = R"({
                               "name": "hello",
                               "value": "world"
                                },
                                {
                               "name": "goodbye",
                               "value": "earth"
                                })";


    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    pluginInfo = createDefaultPluginInfo();

    pluginInfo.addExecutableEnvironmentVariables("goodbye", "earth");

    std::string expected = createJsonString(oldString, newString);
    expected = cleanupStringForCompare(expected);
    std::string result = Common::PluginRegistryImpl::PluginInfo::serializeToString(pluginInfo);
    result = cleanupStringForCompare(result);

    EXPECT_EQ(expected, result);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringWithInvalidDataThrows) //NOLINT
{
    std::string oldString = R"( "xmlTranslatorPath": "path1",)";

    std::string newString = R"( "invalidField": "InvalidData",)";
//
//    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;
//
//    expectedPluginInfo = createDefaultPluginInfo();
//    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_THROW( //NOLINT
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),""),
            Common::PluginRegistryImpl::PluginRegistryException);

}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringThrowsWithIncorrectPluginNameFromFilename) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo = createDefaultPluginInfo();

    EXPECT_THROW( //NOLINT
            Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString("",""),"FOOBAR"),
            Common::PluginRegistryImpl::PluginRegistryException);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringWithInvalidDataTypesThrows) //NOLINT
{

    std::string oldString = R"(  "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ])";


    std::string newString = R"(  "environmentVariables": [
                                      {
                                       "type": "integer",
                                       "name": 0,
                                      }
                                     ])";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_THROW(Common::PluginRegistryImpl::PluginInfo::deserializeFromString(createJsonString(oldString, newString),""), //NOLINT
            Common::PluginRegistryImpl::PluginRegistryException); //NOLINT
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromNonJsonStringThrows) //NOLINT
{
    EXPECT_THROW(Common::PluginRegistryImpl::PluginInfo::deserializeFromString("Non JasonString",""), Common::PluginRegistryImpl::PluginRegistryException); //NOLINT
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromEmptyJsonStringShouldNotThrow) //NOLINT
{
    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::deserializeFromString("{}","sav")); //NOLINT
}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenNoJsonFileFound) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));

    // There may be times when there are no plugin config files (such as initial install)
    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry()); //NOLINT

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenJsonFileFound) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename("/tmp/file1.json");
    files.push_back(filename);
    std::string fileContent = createJsonString("","");


    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename)).WillOnce(Return(fileContent));

    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry()); //NOLINT

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenMultipleJsonFilesFound) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename1("/tmp/file1.json");
    std::string filename2("/tmp/file2.json");
    files.push_back(filename1);
    files.push_back(filename2);
    std::string fileContent = createJsonString("","");

    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Return(fileContent));
    EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return(fileContent));

    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry()); //NOLINT

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenAtLeastOneJsonFileIsValid) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename1("/tmp/file1.json");
    std::string filename2("/tmp/file2.json");
    files.push_back(filename1);
    files.push_back(filename2);
    std::string fileContent = createJsonString("","");

    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Return("invalidJsonContent"));
    EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return(fileContent));

    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry());

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenMultipleJsonFilesAreInvalid) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename1("/tmp/file1.json");
    std::string filename2("/tmp/file2.json");
    files.push_back(filename1);
    files.push_back(filename2);

    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Return("invalidJsonContent"));
    EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return("alsoInvalidJsonContent"));

    // Should not throw if no valid plugin configuration files found.  (Plugs may not have been installed)
    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry());

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenAtLeastOneJsonFileIsSuccessfullyRead) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename1("/tmp/file1.json");
    std::string filename2("/tmp/file2.json");
    files.push_back(filename1);
    files.push_back(filename2);
    std::string fileContent = createJsonString("","");

    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));
    EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return(fileContent));

    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry());

}

TEST_F(PluginRegistryTests, pluginInfoLoadFromDirectoryPathDoesNotThrowWhenAllJsonFilesAreUnsuccessfullyRead) //NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::replaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> files;
    std::string filename1("/tmp/file1.json");
    std::string filename2("/tmp/file2.json");
    files.push_back(filename1);
    files.push_back(filename2);
    std::string fileContent = createJsonString("","");

    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed to read")));
    EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed to read")));

    // When no plugin config files are successfully read, warning should be logged but no expection should be thrown.
    EXPECT_NO_THROW(Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry()); //NOLINT

}

TEST_F(PluginRegistryTests, extractPluginNameReturnsCorrectAnswer) //NOLINT
{
    std::string plugin = Common::PluginRegistryImpl::PluginInfo::extractPluginNameFromFilename("/foo/bar/sav.json");
    EXPECT_EQ(plugin,"sav");
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithValidUserAndGroupStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("root:root");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    EXPECT_EQ(userActual.first, true);
    EXPECT_EQ(userActual.second, 0);

    EXPECT_EQ(groupActual.first, true);
    EXPECT_EQ(groupActual.second, 0);
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithValidUserStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("root");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    EXPECT_EQ(userActual.first, true);
    EXPECT_EQ(userActual.second, 0);

    EXPECT_EQ(groupActual.first, true);
    EXPECT_EQ(groupActual.second, 0);
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithInvalidUserStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("baduser");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    uid_t invalidUser = static_cast<uid_t>(-1);
    gid_t invalidGroup = static_cast<gid_t>(-1);

    EXPECT_EQ(userActual.first, false);
    EXPECT_EQ(userActual.second, invalidUser);

    EXPECT_EQ(groupActual.first, false);
    EXPECT_EQ(groupActual.second, invalidGroup);
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithInvalidUserAndGroupStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("baduser:badgroup");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    uid_t invalidUser = static_cast<uid_t>(-1);
    gid_t invalidGroup = static_cast<gid_t>(-1);

    EXPECT_EQ(userActual.first, false);
    EXPECT_EQ(userActual.second, invalidUser);

    EXPECT_EQ(groupActual.first, false);
    EXPECT_EQ(groupActual.second, invalidGroup);
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithInvalidUserAndValidGroupStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("baduser:root");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    uid_t invalidUser = static_cast<uid_t>(-1);
    gid_t invalidGroup = static_cast<gid_t>(-1);

    EXPECT_EQ(userActual.first, false);
    EXPECT_EQ(userActual.second, invalidUser);

    EXPECT_EQ(groupActual.first, false);
    EXPECT_EQ(groupActual.second, invalidGroup);
}

TEST_F(PluginRegistryTests, pluginInfoSetExecutableUserAndGroupWithValidUserAndInvalidGroupStoresCorrectResults) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo.setExecutableUserAndGroup("root:badgroup");

    std::pair<bool, uid_t> userActual = pluginInfo.getExecutableUser();
    std::pair<bool, uid_t> groupActual = pluginInfo.getExecutableGroup();

    gid_t invalidGroup = static_cast<gid_t>(-1);

    EXPECT_EQ(userActual.first, true);
    EXPECT_EQ(userActual.second, 0);

    EXPECT_EQ(groupActual.first, false);
    EXPECT_EQ(groupActual.second, invalidGroup);
}