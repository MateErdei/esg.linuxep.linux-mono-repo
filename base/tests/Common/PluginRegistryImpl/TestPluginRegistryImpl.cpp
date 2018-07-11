/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Common/FileSystem/IFileSystem.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include <numeric>
#include <Common/PluginRegistryImpl/PluginRegistryException.h>


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
                                     ]
                                    })";

        if(!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);

        }

        return jsonString;
    }


    std::string convertVectorToString(std::vector<std::pair<std::string, std::string>> &value)
    {
        std::string valueAsString;

        for(auto & valueItem : value)
        {
            valueAsString += valueItem.first;
            valueAsString += valueItem.second;
        }

        return valueAsString;
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

        return pluginInfo;
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

        {


            std::vector<std::string> expectedValues = expected.getPolicyAppIds();
            std::vector<std::string> resultedValues = resulted.getPolicyAppIds();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            std::string expectedString;
            expectedString = std::accumulate(expectedValues.begin(), expectedValues.end(), expectedString);
            std::string resultedString;
            resultedString = std::accumulate(resultedValues.begin(), resultedValues.end(), resultedString);

            if( expectedString != resultedString)
            {
                return ::testing::AssertionFailure() << s.str() << " Policy app ids differs: \n expected: "
                                                     <<  expectedString
                                                     << "\n result: " <<  resultedString;
            }

        }

        {
            std::vector<std::string> expectedValues = expected.getStatusAppIds();
            std::vector<std::string> resultedValues = resulted.getStatusAppIds();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            std::string expectedString;
            expectedString = std::accumulate(expectedValues.begin(), expectedValues.end(), expectedString);
            std::string resultedString;
            resultedString = std::accumulate(resultedValues.begin(), resultedValues.end(), resultedString);

            if( expectedString != resultedString)
            {
                return ::testing::AssertionFailure() << s.str() << " Status app ids differs: \n expected: "
                                                     <<  expectedString
                                                     << "\n result: " <<  resultedString;
            }

        }

        {
            std::vector<std::string> expectedValues = expected.getExecutableArguments();
            std::vector<std::string> resultedValues = resulted.getExecutableArguments();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            std::string expectedString;
            expectedString = std::accumulate(expectedValues.begin(), expectedValues.end(), expectedString);
            std::string resultedString;
            resultedString = std::accumulate(resultedValues.begin(), resultedValues.end(), resultedString);

            if( expectedString != resultedString)
            {
                return ::testing::AssertionFailure() << s.str() << " Executable arguments differs: \n expected: "
                                                     <<  expectedString
                                                     << "\n result: " <<  resultedString;
            }

        }

        {
            std::vector<std::pair<std::string, std::string>> expectedValues = expected.getExecutableEnvironmentVariables();
            std::vector<std::pair<std::string, std::string>> resultedValues = resulted.getExecutableEnvironmentVariables();

            std::sort(expectedValues.begin(), expectedValues.end());
            std::sort(resultedValues.begin(), resultedValues.end());

            std::string expectedString = convertVectorToString(expectedValues);
            std::string resultedString = convertVectorToString(resultedValues);

            if( expectedString != resultedString)
            {
                return ::testing::AssertionFailure() << s.str() << " Executable environment variables differs: \n expected: "
                                                     <<  expectedString
                                                     << "\n result: " <<  resultedString;
            }

        }

        return ::testing::AssertionSuccess();
    }

};


// basic tests

TEST_F( PluginRegistryTests, addPolicyAppIdAddsNewValueCorrectly)
{
   Common::PluginRegistryImpl::PluginInfo pluginInfo;

   std::string policyAppId = "AppId1";

    pluginInfo.addPolicyAppIds(policyAppId);

   EXPECT_EQ(pluginInfo.getPolicyAppIds().size(), 1);
   EXPECT_EQ(pluginInfo.getPolicyAppIds()[0], policyAppId);

}


TEST_F( PluginRegistryTests, addStatusAppIdAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string statusAppId = "AppId1";

    pluginInfo.addPolicyAppIds(statusAppId);

    EXPECT_EQ(pluginInfo.getPolicyAppIds().size(), 1);
    EXPECT_EQ(pluginInfo.getPolicyAppIds()[0], statusAppId);
}

TEST_F( PluginRegistryTests, addExecutableArgumentsAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string arg = "Arg1";

    pluginInfo.addExecutableArguments(arg);

    EXPECT_EQ(pluginInfo.getExecutableArguments().size(), 1);
    EXPECT_EQ(pluginInfo.getExecutableArguments()[0], arg);
}

TEST_F( PluginRegistryTests, addExecutableEnvironmentVariablesAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string envName = "envName1";
    std::string envValue = "envValue1";

    pluginInfo.addExecutableEnvironmentVariables(envName, envValue);

    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables().size(), 1);
    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables()[0].first, envName);
    EXPECT_EQ(pluginInfo.getExecutableEnvironmentVariables()[0].second, envValue);
}

TEST_F( PluginRegistryTests, setExecutableFullPathAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string path = "/tmp/some-path/some-exe";

    pluginInfo.setExecutableFullPath(path);

    EXPECT_EQ(pluginInfo.getExecutableFullPath(), path);
}

TEST_F( PluginRegistryTests, setXmlTranslatorPathAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string path = "/tmp/some-path/some-exe";

    pluginInfo.setXmlTranslatorPath(path);

    EXPECT_EQ(pluginInfo.getXmlTranslatorPath(), path);
}

TEST_F( PluginRegistryTests, setPluginNameAddsNewValueCorrectly)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    std::string name = "pluginName";

    pluginInfo.setPluginName(name);

    EXPECT_EQ(pluginInfo.getPluginName(), name);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoData)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsEmptyString)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({""});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("app1","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsEmpty)
{
    std::string oldString = R"("app1")";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPolicyAppIdIsMissing)
{
        std::string oldString = R"("policyAppIds": [
                                    "app1"
                                     ],)";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPolicyAppIds({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}


TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsEmptyString)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({""});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("app2","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsEmpty)
{
    std::string oldString = R"("app2")";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenStatusAppIdIsMissing)
{
    std::string oldString = R"(                                 "statusAppIds": [
                                      "app2"
                                     ],)";


    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setStatusAppIds({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPluginNameIsEmptyString)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPluginName("");

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("PluginName","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenPluginNameIsMissing)
{
    std::string oldString = R"("pluginName": "PluginName",)";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setPluginName("");

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString,newString)));
}


TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenXmlTranslatorPathIsEmptyString)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("path1","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenXmlTranslatorPathIsMissing)
{
    std::string oldString = R"( "xmlTranslatorPath": "path1",)";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenExecutableArgsIsEmpty)
{
    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableArguments({""});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("arg1","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenExecutableArgsIsMissing)
{
    std::string oldString = R"(     "executableArguments": [
                                      "arg1"
                                     ],)";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableArguments({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenEnvironmentVariablesIsEmpty)
{
    std::string oldString = R"({
                               "name": "hello",
                               "value": "world"
                                })";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableArguments({""});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString("arg1","")));
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringReturnsExpectedPluginInfoDataWhenEnvironmentVariablesIsMissing)
{
    std::string oldString = R"(  "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ])";

    std::string newString = "";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setExecutableEnvironmentVariables({});

    EXPECT_PRED_FORMAT2( pluginInfoSimilar, expectedPluginInfo, expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)));
}

TEST_F( PluginRegistryTests, pluginInfoSerializeToStringReturnsExpectedData)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;

    pluginInfo = createDefaultPluginInfo();

    std::string expected = createJsonString("","");
    expected = cleanupStringForCompare(expected);
    std::string result = pluginInfo.serializeToString(pluginInfo);
    result = cleanupStringForCompare(result);

    EXPECT_EQ(expected, result);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringWithInvalidDataThrows)
{
    std::string oldString = R"( "xmlTranslatorPath": "path1",)";

    std::string newString = R"( "invalidField": "InvalidData",)";

    Common::PluginRegistryImpl::PluginInfo expectedPluginInfo;

    expectedPluginInfo = createDefaultPluginInfo();
    expectedPluginInfo.setXmlTranslatorPath("");

    EXPECT_THROW(expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)), Common::PluginRegistryImpl::PluginRegistryException);

}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromStringWithInvalidDataTypesThrows)
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

    EXPECT_THROW(expectedPluginInfo.deserializeFromString(createJsonString(oldString, newString)), Common::PluginRegistryImpl::PluginRegistryException);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromNonJsonStringThrows)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo = createDefaultPluginInfo();


    EXPECT_THROW(pluginInfo.deserializeFromString("Non JasonString"), Common::PluginRegistryImpl::PluginRegistryException);
}

TEST_F( PluginRegistryTests, pluginInfoDeserializeFromEmptyJsonStringShouldNotThrow)
{
    Common::PluginRegistryImpl::PluginInfo pluginInfo;
    pluginInfo = createDefaultPluginInfo();

    EXPECT_NO_THROW(pluginInfo.deserializeFromString("{}"));
}
