// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "MockThreatDetectorResources.h"

#include "common/MemoryAppender.h"
#include <gtest/gtest.h>


using namespace sspl::sophosthreatdetectorimpl;
using namespace testing;

namespace {
    class TestSophosThreatDetectorMain : public MemoryAppenderUsingTests//, public SophosThreatDetectorMain
    {
    public:
        TestSophosThreatDetectorMain() :  MemoryAppenderUsingTests("SophosThreatDetectorImpl")
        {}

        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
                appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSophosThreatDetectorMain");

                m_MockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>();
        }

        std::shared_ptr<NiceMock<MockThreatDetectorResources>> m_MockThreatDetectorResources;
    };

}

TEST_F(TestSophosThreatDetectorMain, construction)
{
    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);
}


