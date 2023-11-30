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
    auto result = Plugin::ActionProcessor::processIsolateAction("");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, nonXml)
{
    auto result = Plugin::ActionProcessor::processIsolateAction("{{{{{");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, invalidXml)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, nonActionElement)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <foobar type="sophos.mgt.action.IsolationRequest">
                 <enabled>true</enabled>
    </foobar>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, differentAction)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="foobar">
                 <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, missingEnabledElement)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, invalidEnabledElement)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled></enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, duplicateEnabledElement)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
         <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
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
