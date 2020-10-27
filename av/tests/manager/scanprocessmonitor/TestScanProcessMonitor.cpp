/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "datatypes/sophos_filesystem.h"
#include "modules/manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "ScanProcessMonitorMemoryAppenderUsingTests.h"
#include "modules/common/ThreadRunner.h"

#include <fstream>

using namespace plugin::manager::scanprocessmonitor;
using ScanProcessMonitorPtr = std::unique_ptr<ScanProcessMonitor>;
namespace fs = sophos_filesystem;

namespace
{
    class TestScanProcessMonitor : public ScanProcessMonitorMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestScanProcessMonitor, constructionWithExistingPath) // NOLINT
{
    EXPECT_NO_THROW(auto m = std::make_unique<ScanProcessMonitor>("/bin/bash"));
}

TEST_F(TestScanProcessMonitor, constructWithNonExistentFile) // NOLINT
{
    try
    {
        auto m = std::make_unique<ScanProcessMonitor>("this config does not exists");
        FAIL() << "Did not throw exception as expected";
    }
    catch (std::runtime_error& e)
    {
        ASSERT_EQ(e.what(),std::string("SCANNER_PATH doesn't refer to a file"));
    }
    catch (...)
    {
        FAIL() <<  "Unexpected exception type";
    }
}

TEST_F(TestScanProcessMonitor, constructWithEmptyConfig) // NOLINT
{
    fs::create_directories("sandbox/a/b/");
    std::ofstream("sandbox/a/b/empty.config");

    auto p = fs::path("");

    try
    {
        auto m = std::make_unique<ScanProcessMonitor>(p);
        FAIL() << "Did not throw exception as expected";
    }
    catch (std::runtime_error& e)
    {
        ASSERT_EQ(e.what(),std::string("SCANNER_PATH is empty in arguments/configuration"));
    }
    catch (...)
    {
        FAIL() <<  "Unexpected exception type";
    }
}
TEST_F(TestScanProcessMonitor, testRunner) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");

    UsingMemoryAppender memoryAppenderHolder(*this);

    try
    {
        ThreadRunner sophos_thread_detector(*m, "scanProcessMonitor");
        EXPECT_TRUE(waitForLog("Starting \"/bin/bash\"", 500ms));
        sophos_thread_detector.killThreads();
        EXPECT_TRUE(waitForLog("Exiting sophos_threat_detector monitor", 500ms));
    }
    catch (std::exception& e)
    {
        FAIL() << e.what();
    }
}

// closest thing to terminating ScanProcessMonitor asap
TEST_F(TestScanProcessMonitor, testRunnerCallTerminateImmediately) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");

    UsingMemoryAppender memoryAppenderHolder(*this);

    try
    {
        ThreadRunner sophos_thread_detector(*m, "scanProcessMonitor");
        sophos_thread_detector.killThreads();
        EXPECT_TRUE(waitForLog("Exiting sophos_threat_detector monitor", 500ms));
    }
    catch (std::exception& e)
    {
        FAIL() << e.what();
    }
}

