// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include <gtest/gtest.h>
#include "Common/Logging/SophosLoggerMacros.h"

#include "avscanner/avscannerimpl/Options.h"

using namespace avscanner::avscannerimpl;

TEST(Options, TestNoArgs)
{
    int argc = 0;
    char* argv[0];
    Options o(argc, argv);
    EXPECT_EQ(o.paths().size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestPaths)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestMultiplePaths)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestConfig)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestArchiveScanning)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}


TEST(Options, TestImageScanning)
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--scan-images";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_TRUE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestEnablePUAdetections)
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--detect-puas";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.imageScanning());
    EXPECT_TRUE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestFollowSymlinks)
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--follow-symlinks";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "");
    EXPECT_FALSE(o.archiveScanning());
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_TRUE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestHelp)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_TRUE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
}

TEST(Options, TestExclusions)
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
    EXPECT_FALSE(o.imageScanning());
    EXPECT_FALSE(o.detectPUAs());
    EXPECT_FALSE(o.followSymlinks());
    EXPECT_FALSE(o.help());
    auto exclusions = o.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0), "file.txt");
}

TEST(Options, TestOutput)
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

TEST(Options, TestLogLevel)
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

TEST(Options, TestShortArguments)
{
    std::string logFile = "scan.log";
    const int argc = 17;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "-c";
    argv[2] = "/bar";
    argv[3] = "-a";
    argv[4] = "-i";
    argv[5] = "-s";
    argv[6] = "-h";
    argv[7] = "-x";
    argv[8] = "file.txt";
    argv[9] = "-f";
    argv[10] = "/foo";
    argv[11] = "-o";
    argv[12] = logFile.c_str();
    argv[13] = "-l";
    argv[14] = "DEBUG";
    argv[15] = "--";
    argv[16] = "/baz";
    Options o(argc, const_cast<char**>(argv));
    EXPECT_EQ(o.config(), "/bar");
    EXPECT_TRUE(o.archiveScanning());
    EXPECT_TRUE(o.imageScanning());
    EXPECT_TRUE(o.followSymlinks());
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

TEST(Options, TestGetHelp)
{
    const int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--help";
    Options o(argc, const_cast<char**>(argv));
    EXPECT_NE(Options::getHelp(), "");
}

TEST(Options, TestVerifyLogLevelUppercase)
{
    EXPECT_EQ(Options::verifyLogLevel("OFF"), log4cplus::OFF_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("DEBUG"), log4cplus::DEBUG_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("INFO"), log4cplus::INFO_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("SUPPORT"), log4cplus::SUPPORT_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("WARN"), log4cplus::WARN_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("ERROR"), log4cplus::ERROR_LOG_LEVEL);
}

TEST(Options, TestVerifyLogLevelLowercase)
{
    EXPECT_EQ(Options::verifyLogLevel("off"), log4cplus::OFF_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("debug"), log4cplus::DEBUG_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("info"), log4cplus::INFO_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("support"), log4cplus::SUPPORT_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("warn"), log4cplus::WARN_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("error"), log4cplus::ERROR_LOG_LEVEL);
}

TEST(Options, TestVerifyLogLevelCapitalised)
{
    EXPECT_EQ(Options::verifyLogLevel("Off"), log4cplus::OFF_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("Debug"), log4cplus::DEBUG_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("Info"), log4cplus::INFO_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("Support"), log4cplus::SUPPORT_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("Warn"), log4cplus::WARN_LOG_LEVEL);
    EXPECT_EQ(Options::verifyLogLevel("Error"), log4cplus::ERROR_LOG_LEVEL);
}

TEST(Options, TestVerifyLogLevelThrowsException)
{
    EXPECT_THROW(Options::verifyLogLevel("this is going to throw"), po::error);
}

TEST(Options, testPUAExclusions)
{
    constexpr int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--exclude-puas";
    argv[2] = "FOO-PUA";
    Options o(argc, const_cast<char**>(argv));
    auto exclusions = o.puaExclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0), "FOO-PUA");
}

TEST(Options, testMultiplePUAExclusions)
{
    constexpr int argc = 4;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--exclude-puas";
    argv[2] = "FOO-PUA";
    argv[3] = "another123";
    Options o(argc, const_cast<char**>(argv));
    auto exclusions = o.puaExclusions();
    ASSERT_EQ(exclusions.size(), 2);
    EXPECT_EQ(exclusions.at(0), "FOO-PUA");
    EXPECT_EQ(exclusions.at(1), "another123");
}

TEST(Options, testPUAExclusionsWithoutExclusion)
{
    constexpr int argc = 2;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--exclude-puas";
    EXPECT_THROW(Options(argc, const_cast<char**>(argv)), boost::program_options::error);
}
