// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/ActionProcessor.h"

#include "base/tests/Common/Helpers/MemoryAppender.h"

#include "pluginimpl/config.h"

#include <gtest/gtest.h>

namespace
{
    class TestActionProcessor : public MemoryAppenderUsingTests
    {
    public:
        TestActionProcessor() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {}
    };
}

TEST_F(TestActionProcessor, emptyAction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction("");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Received empty action!"));
}

TEST_F(TestActionProcessor, nonXml)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction("{{{{{");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Ignoring non-XML action: {{{{{"));
}

TEST_F(TestActionProcessor, invalidXml)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Failed to parse action: "));
}

TEST_F(TestActionProcessor, largeInvalidXML)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto longStr = std::string(30000, 't');
    auto result = Plugin::ActionProcessor::processIsolateAction(longStr);
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Ignoring non-XML action: " + longStr));
}


TEST_F(TestActionProcessor, largeActionXML)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto policyStr = R"(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
                 <enabled>)" + std::string(30000, 't') + R"(true</enabled>
    </action>)";
    auto result = Plugin::ActionProcessor::processIsolateAction(policyStr);
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Contents of enabled element are invalid: "));
}


TEST_F(TestActionProcessor, nonActionElement)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <foobar type="sophos.mgt.action.IsolationRequest">
                 <enabled>true</enabled>
    </foobar>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Incorrect action type: "));
}

TEST_F(TestActionProcessor, differentAction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="foobar">
                 <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Incorrect action type: "));
}

TEST_F(TestActionProcessor, missingEnabledElement)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("No action or enabled elements"));
}

TEST_F(TestActionProcessor, invalidEnabledElement)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled></enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Contents of enabled element are invalid: "));
}

TEST_F(TestActionProcessor, invalidContentsOfEnabledElement)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>not true or false</enabled>
    </action>)SOPHOS");
    ASSERT_EQ(result, std::nullopt);
}

TEST_F(TestActionProcessor, additionalFields)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <anotherField>abc</anotherField>
         <enabled>true</enabled>
         <field3>false</field3>
    </action>)SOPHOS");
    ASSERT_NE(result, std::nullopt);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(TestActionProcessor, duplicateEnabledElement)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
         <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
    EXPECT_TRUE(appenderContains("Multiple enabled elements!"));
}

TEST_F(TestActionProcessor, enableIsolation)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
    </action>)SOPHOS");
    ASSERT_NE(result, std::nullopt);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(TestActionProcessor, disableIsolation)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>false</enabled>
    </action>)SOPHOS");
    ASSERT_NE(result, std::nullopt);
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value());
}
