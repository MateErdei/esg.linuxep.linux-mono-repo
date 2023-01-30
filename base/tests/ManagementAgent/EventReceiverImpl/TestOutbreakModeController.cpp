// Copyright 2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/MemoryAppender.h"

#include "ManagementAgent/EventReceiverImpl/OutbreakModeController.h"

using namespace ManagementAgent::EventReceiverImpl;

namespace
{
    class TestOutbreakModeController : public MemoryAppenderUsingTests
    {
    public:
        TestOutbreakModeController()
            : MemoryAppenderUsingTests("managementagent")
        {}
    };
}

TEST_F(TestOutbreakModeController, construction)
{
    EXPECT_NO_THROW(std::make_shared<OutbreakModeController>());
}

TEST_F(TestOutbreakModeController, outbreak_mode_logged)
{
    const std::string detection_xml = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/path/to/eicar.txt</path>
  </alert>
</event>)sophos";

    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<101; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", detection_xml);
    }

    EXPECT_TRUE(waitForLog("Entering outbreak mode"));
}