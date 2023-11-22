// Copyright 2023 Sophos All rights reserved.

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

TEST_F(TestActionProcessor, empty_action)
{
    auto result = Plugin::ActionProcessor::processIsolateAction("");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, non_xml)
{
    auto result = Plugin::ActionProcessor::processIsolateAction("{{{{{");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, invalid_xml)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, non_action_element)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <foobar type="sophos.mgt.action.IsolationRequest">
                 <enabled>true</enabled>
    </foobar>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, different_action)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="foobar">
                 <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, missing_enabled_element)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, invalid_enabled_element)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled></enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, duplicate_enabled_element)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
         <enabled>true</enabled>
    </action>)SOPHOS");
    EXPECT_EQ(result, std::nullopt);
    EXPECT_FALSE(result);
}

TEST_F(TestActionProcessor, enable_isolation)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
    </action>)SOPHOS");
    ASSERT_NE(result, std::nullopt);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(TestActionProcessor, disable_isolation)
{
    auto result = Plugin::ActionProcessor::processIsolateAction(R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>false</enabled>
    </action>)SOPHOS");
    ASSERT_NE(result, std::nullopt);
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value());
}
