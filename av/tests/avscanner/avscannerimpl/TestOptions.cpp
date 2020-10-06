/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

#include "avscanner/avscannerimpl/Options.h"

using namespace avscanner::avscannerimpl;

TEST(Options, TestNoArgs) // NOLINT
{
    int argc = 0;
    char* argv[0];
    Options o(argc, argv);
    EXPECT_EQ(o.paths().size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestPaths) // NOLINT
{
    int argc = 2;
    const char* argv[2];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "/foo";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths.at(0), "/foo");
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestMultiplePaths) // NOLINT
{
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "/foo";
    argv[2] = "/bar";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths.at(0), "/foo");
    EXPECT_EQ(paths.at(1), "/bar");
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestConfig) // NOLINT
{
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--config";
    argv[2] = "/bar";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "/bar");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestArchiveScanning) // NOLINT
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--scan-archives";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_TRUE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestHelp) // NOLINT
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--help";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_TRUE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestExclusions) // NOLINT
{
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--exclude";
    argv[2] = "file.txt";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0), "file.txt");
}

TEST(Options, TestOutput) // NOLINT
{
    std::string expectedLogFile = "scan.log";
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--output";
    argv[2] = expectedLogFile.c_str();
    Options o(argc, const_cast<char**>(argv));
    EXPECT_EQ(o.logFile(), expectedLogFile);
}

TEST(Options, TestLogLevel) // NOLINT
{
    std::string logLevel = "INFO";
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--log-level";
    argv[2] = logLevel.c_str();
    Options o(argc, const_cast<char**>(argv));
    EXPECT_EQ(o.logLevel(), log4cplus::INFO_LOG_LEVEL);
}

TEST(Options, TestShortArguments) // NOLINT
{
    std::string logFile = "scan.log";
    const int argc = 15;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "-c";
    argv[2] = "/bar";
    argv[3] = "-s";
    argv[4] = "-h";
    argv[5] = "-x";
    argv[6] = "file.txt";
    argv[7] = "-f";
    argv[8] = "/foo";
    argv[9] = "-o";
    argv[10] = logFile.c_str();
    argv[11] = "-l";
    argv[12] = "DEBUG";
    argv[13] = "--";
    argv[14] = "/baz";
    Options o(argc, const_cast<char**>(argv));
    EXPECT_EQ(o.config(), "/bar");
    EXPECT_TRUE(o.archiveScanning());
    EXPECT_TRUE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0), "file.txt");
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths.at(0), "/foo");
    EXPECT_EQ(paths.at(1), "/baz");
    EXPECT_EQ(o.logFile(), logFile);
    EXPECT_EQ(o.logLevel(), log4cplus::DEBUG_LOG_LEVEL);
}

TEST(Options, TestGetHelp) // NOLINT
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--help";
    Options o(argc, const_cast<char**>(argv));
    EXPECT_NE(Options::getHelp(), "");
}

TEST(Options, TestVerifyLogLevel) // NOLINT
{
    EXPECT_EQ(Options::verifyLogLevel("OFF"), log4cplus::OFF_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("DEBUG"), log4cplus::DEBUG_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("INFO"), log4cplus::INFO_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("SUPPORT"), log4cplus::SUPPORT_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("WARN"), log4cplus::WARN_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("ERROR"), log4cplus::ERROR_LOG_LEVEL);
}