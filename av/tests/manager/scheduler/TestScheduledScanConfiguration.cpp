/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/



#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"

using namespace manager::scheduler;

TEST(ScheduledScanConfiguration, constructionWithArg) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml></xml>");
    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
}
