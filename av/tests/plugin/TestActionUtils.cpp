// Copyright 2022, Sophos Limited.  All rights reserved.

#include <pluginimpl/ActionUtils.h>

#include "tests/common/LogInitializedTests.h"
#include <Common/XmlUtilities/AttributesMap.h>

namespace
{
    class TestActionUtils : public LogInitializedTests
    {
    };
}

TEST_F(TestActionUtils, isScanNowActionReturnTrueOnScanNowAction)
{
    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";
    auto attributeMap = Common::XmlUtilities::parseXml(actionXml);
    EXPECT_TRUE(pluginimpl::isScanNowAction(attributeMap));
}

TEST_F(TestActionUtils, isScanNowActionReturnFalseOnScanNowAction)
{
    std::string actionXml =
        R"(<action type="sophos.mgt.action.SAVClearFromList" xmlns="com.sophos/msys/action">
<threat-set>
     <threat type="{{threatType}}" typeId="{{threatTypeId}}" name="{{threatName}}" item="{{threatFilename}}" id="{{threatId}}" idSource="NameFilenameFilepathCIMD5"></threat>
</threat-set>
</action>)";
    auto attributeMap = Common::XmlUtilities::parseXml(actionXml);
    EXPECT_FALSE(pluginimpl::isScanNowAction(attributeMap));
}

TEST_F(TestActionUtils, isSAVClearActionReturnTrueOnSAVClearAction)
{
    std::string actionXml =
        R"(<action type="sophos.mgt.action.SAVClearFromList" xmlns="com.sophos/msys/action">
<threat-set>
     <threat type="{{threatType}}" typeId="{{threatTypeId}}" name="{{threatName}}" item="{{threatFilename}}" id="{{threatId}}" idSource="NameFilenameFilepathCIMD5"></threat>
</threat-set>
</action>)";
    auto attributeMap = Common::XmlUtilities::parseXml(actionXml);
    EXPECT_TRUE(pluginimpl::isSAVClearAction(attributeMap));
}