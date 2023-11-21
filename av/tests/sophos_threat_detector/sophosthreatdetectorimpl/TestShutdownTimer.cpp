// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"

#include <fstream>

// test headers
#include "tests/common/Common.h"
#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace testing;

namespace
{
    class TestShutdownTimer : public LogInitializedTests
    {
    protected:
        void TearDown() override
        {
            //fs::remove_all(tmpdir());
        }
    };
}

TEST_F(TestShutdownTimer, testDefaultTimeout) // NOLINT
{
    fs::path nonexistentConfigFile = "";
    sspl::sophosthreatdetectorimpl::ShutdownTimer timer(nonexistentConfigFile);
    time_t defaultTimeout = timer.timeout();
    // Allow timeout to be 1 second less than default in case timeout() call happens in the next second interval
    EXPECT_GE(defaultTimeout, 3600 - 1);
}

TEST_F(TestShutdownTimer, testConfiguredTimeout) // NOLINT
{
    fs::path testdir = tmpdir();
    fs::create_directories(testdir);
    fs::path configFile = testdir / "threat_detector_config";
    std::ofstream configFileHandle;
    configFileHandle.open(configFile);
    configFileHandle << "{\"shutdownTimeout\":60}";
    configFileHandle.close();

    sspl::sophosthreatdetectorimpl::ShutdownTimer timer(configFile);
    time_t defaultTimeout = timer.timeout();
    // Allow timeout to be 1 second less than default in case timeout() call happens in the next second interval
    EXPECT_GE(defaultTimeout, 60 - 1);
}

