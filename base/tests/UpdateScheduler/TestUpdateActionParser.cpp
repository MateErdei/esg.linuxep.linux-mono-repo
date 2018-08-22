/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/XmlUtilities/AttributesMap.h>
#include <gmock/gmock-matchers.h>
#include <UpdateSchedulerImpl/configModule/UpdateActionParser.h>

static std::string updateAction{R"sophos(<?xml version='1.0'?>

<action type="sophos.mgt.action.ALCForceUpdate"/>)sophos"};

static std::string invalidUpdateAction{R"sophos(<?xml version='1.0'?>
<action type="sophos.mgt.action.ALCUpdate"/>)sophos"};

TEST(TestUpdatePolicyTranslator, ParseUpdateAction) // NOLINT
{
    EXPECT_TRUE(UpdateSchedulerImpl::isUpdateNowAction(updateAction));
    EXPECT_FALSE(UpdateSchedulerImpl::isUpdateNowAction(invalidUpdateAction));
    //Invalid XML
    EXPECT_FALSE(UpdateSchedulerImpl::isUpdateNowAction(""));
}