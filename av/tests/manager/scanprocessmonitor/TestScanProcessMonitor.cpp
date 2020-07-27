/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include "datatypes/sophos_filesystem.h"
#include "modules/pluginimpl/Logger.h"
#include "modules/manager/scanprocessmonitor/ScanProcessMonitor.h"

#include <fstream>

using namespace plugin::manager::scanprocessmonitor;
using ScanProcessMonitorPtr = std::unique_ptr<ScanProcessMonitor>;
namespace fs = sophos_filesystem;

class ThreadRunner
{
public:
    explicit ThreadRunner(Common::Threads::AbstractThread& thread, std::string name)
            : m_thread(thread), m_name(std::move(name))
    {
        LOGINFO("Starting " << m_name);
        m_thread.start();
    }

    ~ThreadRunner()
    {
        LOGINFO("Stopping " << m_name);
        m_thread.requestStop();
        LOGINFO("Joining " << m_name);
        m_thread.join();
    }

    Common::Threads::AbstractThread& m_thread;
    std::string m_name;
};

TEST(TestScanProcessMonitor, constructionWithArg) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");
}

TEST(TestScanProcessMonitor, constructWithNonExistentFile) // NOLINT
{
    try
    {
        auto m = std::make_unique<ScanProcessMonitor>("this config does not exists");
    }
    catch (std::runtime_error& e)
    {
        ASSERT_EQ(e.what(),std::string("SCANNER_PATH doesn't refer to a file"));
    }
}

TEST(TestScanProcessMonitor, constructWithEmptyConfig) // NOLINT
{
    fs::create_directories("sandbox/a/b/");
    std::ofstream("sandbox/a/b/empty.config");

    auto p = fs::path("");

    try
    {
        auto m = std::make_unique<ScanProcessMonitor>(p);
    }
    catch (std::runtime_error& e)
    {
        ASSERT_EQ(e.what(),std::string("SCANNER_PATH is empty in arguments/configuration"));
    }
}
TEST(TestScanProcessMonitor, testRunner) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");

    try
    {
        ThreadRunner sophos_thread_detector(*m, "scanProcessMonitor");
        sleep(2);
        sophos_thread_detector.~ThreadRunner();
    }
    catch (std::exception& e)
    {
        FAIL() << e.what();
    }
}

// closest thing to terminating the monitor asap
TEST(TestScanProcessMonitor, testRunnerCallTerminateImmediately) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");

    try
    {
        ThreadRunner sophos_thread_detector(*m, "scanProcessMonitor");
        sophos_thread_detector.~ThreadRunner();
    }
    catch (std::exception& e)
    {
        FAIL() << e.what();
    }
}

