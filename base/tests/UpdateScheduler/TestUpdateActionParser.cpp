// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/XmlUtilities/AttributesMap.h"
#include "UpdateSchedulerImpl/configModule/UpdateActionParser.h"
#include <gmock/gmock-matchers.h>

static std::string updateAction{ R"sophos(<?xml version='1.0'?>

<action type="sophos.mgt.action.ALCForceUpdate"/>)sophos" };

static std::string invalidUpdateAction{ R"sophos(<?xml version='1.0'?>
<action type="sophos.mgt.action.ALCUpdate"/>)sophos" };

TEST(TestUpdatePolicy, ParseUpdateAction)
{
    using namespace UpdateSchedulerImpl::configModule;
    EXPECT_TRUE(isUpdateNowAction(updateAction));
    EXPECT_FALSE(isUpdateNowAction(invalidUpdateAction));
    // Invalid XML
    EXPECT_FALSE(isUpdateNowAction(""));
}