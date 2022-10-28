// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "common/MemoryAppender.h"
#include <gtest/gtest.h>

using namespace ::testing;
using namespace sspl::sophosthreatdetectorimpl;

namespace {
    class TestSophosThreatDetectorMain : public MemoryAppenderUsingTests
    {
    public:
        TestSophosThreatDetectorMain() :  MemoryAppenderUsingTests("SophosThreatDetectorImpl") {}

        void SetUp() override
        {
                auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
                appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSophosThreatDetectorMain");
        }
    };

}

TEST_F(TestSophosThreatDetectorMain, construction)
{
    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();

}


