// Copyright 2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

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

    const int OUTBREAK_COUNT = 100;

    const std::string DETECTION_XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/path/to/eicar.txt</path>
  </alert>
</event>)sophos";
}

TEST_F(TestOutbreakModeController, construction)
{
    EXPECT_NO_THROW(std::make_shared<OutbreakModeController>());
}

TEST_F(TestOutbreakModeController, outbreak_mode_logged)
{
    const std::string detection_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", detection_xml);
    }

    EXPECT_TRUE(waitForLog("Entering outbreak mode"));
}


TEST_F(TestOutbreakModeController, do_not_enter_outbreak_mode_for_cleanups)
{
    const std::string event_xml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="1" origin="1">
    <items totalItems="1">
      <item type="file" result="0">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
}

TEST_F(TestOutbreakModeController, insufficient_alerts_do_not_enter_outbreak_mode)
{
    const std::string event_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
}


TEST_F(TestOutbreakModeController, alerts_over_two_days_do_not_enter_outbreak_mode)
{
    const std::string event_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();

    auto now = OutbreakModeController::clock_t::now();
    auto yesterday = now - std::chrono::hours{24};

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, yesterday);
    }

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, now);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
}
