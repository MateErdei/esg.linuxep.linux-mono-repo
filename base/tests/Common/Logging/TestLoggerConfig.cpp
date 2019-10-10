/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/Logging/SophosLoggerMacros.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>

#include <iostream>
#include <thread>
#include <unistd.h>

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(logger, x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(logger, x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(logger, x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(logger, x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(logger, x)     // NOLINT

#ifndef ARTISANBUILD

struct TestInput
{
    std::string logConfig{};
    int productContainLogs = 0;
    int productDoesNotContainLogs = 0;
    int moduleContainLogs = 0;
    int moduleDoesNotContainLogs = 0;
    std::string rootPath{};
};
enum LogLevels : int
{
    DEBUG = 0x01,
    INFO = 0x02,
    SUPPORT = 0x04,
    WARN = 0x08,
    ERROR = 0x10
};

class TestLoggerConfig : public ::testing::Test
{
public:
    TestLoggerConfig()
    {
        // ensure google run the test in a thread safe way
        testing::FLAGS_gtest_death_test_style = "threadsafe";
    }

    static void SetUpTestCase() { testRunPath.reset(new Tests::TempDir("/tmp", "sspl-log")); }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase() { testRunPath.reset(); }
    static std::unique_ptr<Tests::TempDir> testRunPath;
};
std::unique_ptr<Tests::TempDir> TestLoggerConfig::testRunPath;

/**
 * Log4cplus uses static loggers (singletons) that outlives the run of individual tests in google test. Hence,
 * it is necessary to ensure that tests are executed in its own processes, using google DEATH_TEST
 * see https://github.com/google/googletest/blob/master/googletest/docs/advanced.md
 */
class TestLoggerConfigForDeathTest
{
public:
    TestLoggerConfigForDeathTest(std::string rootpath) :
        m_logConfigPath("base/etc/logger.conf"),
        tempDir(rootpath, "testlog")
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    }

    void setLogConfig(const std::string& content) { tempDir.createFile(m_logConfigPath, content); }

    void runTest(const std::string& productName, const std::string& componentName)
    {
        std::string logspath = Common::FileSystem::join("logs/base", productName + ".log");
        // the default operation of loggers is to append. Hence, for the test it has to be cleared.
        if (tempDir.exists(logspath))
        {
            Common::FileSystem::fileSystem()->removeFile(tempDir.absPath(logspath));
        }

        Common::Logging::FileLoggingSetup setup(productName);
        auto logger = Common::Logging::getInstance(productName);
        LOGDEBUG("productname debug");
        LOGINFO("productname info");
        LOGSUPPORT("productname support");
        LOGWARN("productname warn");
        LOGERROR("productname error");
        logger = Common::Logging::getInstance(componentName);
        LOGDEBUG("componentname debug");
        LOGINFO("componentname info");
        LOGSUPPORT("componentname support");
        LOGWARN("componentname warn");
        LOGERROR("componentname error");
        LOG4CPLUS_FATAL(logger, "TESTEND");
    }

    std::string getLogContent(const std::string& productName)
    {
        std::string logspath = Common::FileSystem::join("logs/base", productName + ".log");
        std::string content;
        int count = 0;
        do
        {
            count++;
            try
            {
                content = tempDir.fileContent(logspath);
                if (content.find("TESTEND") != std::string::npos)
                {
                    return content;
                }
            }
            catch (std::exception&)
            {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (count < 10);
        return content;
    }

    std::string m_logConfigPath;
    Tests::TempDir tempDir;
    std::string m_logContent;

    void productShould(const std::string& content, int containLogs, int notContainLogs)
    {
        logShouldContain(content, "productname", containLogs);
        logShouldNotContain(content, "productname", notContainLogs);
    }

    void moduleShould(const std::string& content, int containLogs, int notContainLogs)
    {
        logShouldContain(content, "componentname", containLogs);
        logShouldNotContain(content, "componentname", notContainLogs);
    }

    void operator()(TestInput testInput)
    {
        std::string productName{ "testlogging" };
        std::string module{ "module" };
        setLogConfig(testInput.logConfig);
        runTest(productName, module);
        m_logContent = getLogContent(productName);

        productShould(m_logContent, testInput.productContainLogs, testInput.productDoesNotContainLogs);
        moduleShould(m_logContent, testInput.moduleContainLogs, testInput.moduleDoesNotContainLogs);
    }

private:
    void logShouldContain(const std::string& content, const std::string& logname, int logLevels)
    {
        if (logLevels & LogLevels::DEBUG)
        {
            EXPECT_TRUE(contains(content, logname + " debug"));
        }
        if (logLevels & LogLevels::INFO)
        {
            EXPECT_TRUE(contains(content, logname + " info"));
        }
        if (logLevels & LogLevels::SUPPORT)
        {
            EXPECT_TRUE(contains(content, logname + " support"));
            EXPECT_TRUE(contains(content, "SPRT")) << content;
        }
        if (logLevels & LogLevels::WARN)
        {
            EXPECT_TRUE(contains(content, logname + " warn"));
        }
        if (logLevels & LogLevels::ERROR)
        {
            EXPECT_TRUE(contains(content, logname + " error"));
        }
    }

    void logShouldNotContain(const std::string& content, const std::string& logname, int logLevels)
    {
        if (logLevels & LogLevels::DEBUG)
        {
            EXPECT_FALSE(contains(content, logname + " debug"));
        }
        if (logLevels & LogLevels::INFO)
        {
            EXPECT_FALSE(contains(content, logname + " info"));
        }
        if (logLevels & LogLevels::SUPPORT)
        {
            EXPECT_FALSE(contains(content, logname + " support"))
                << content << "\nShould not contain support for " << logname;
        }
        if (logLevels & LogLevels::WARN)
        {
            EXPECT_FALSE(contains(content, logname + " warn"));
        }
        if (logLevels & LogLevels::ERROR)
        {
            EXPECT_FALSE(contains(content, logname + " error"));
        }
    }

    bool contains(const std::string& content, const std::string& needle)
    {
        bool v = content.find(needle) != std::string::npos;
        return v;
    }
};

void runTest(TestInput testInput, bool inNewProc = true)
{
    TestLoggerConfigForDeathTest testLoggerConfigForDeathTest(testInput.rootPath);
    testLoggerConfigForDeathTest(testInput);
    if (!inNewProc)
    {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (::testing::Test::HasFailure())
    {
        std::cerr << testLoggerConfigForDeathTest.m_logContent;
        exit(1);
    }
    else
    {
        std::cerr << "Success";
        exit(0);
    }
}

TEST_F(TestLoggerConfig, GlobalSupportLogWrittenToFile) // NOLINT
{
    TestInput testInput{ .logConfig = R"(
[global]
VERBOSITY=SUPPORT
)",
                         .productContainLogs =
                             LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN | LogLevels::SUPPORT,
                         .productDoesNotContainLogs = LogLevels::DEBUG,
                         .moduleContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN | LogLevels::SUPPORT,
                         .moduleDoesNotContainLogs = LogLevels::DEBUG,
                         .rootPath = TestLoggerConfig::testRunPath->dirPath() };

    // swap the comment lines below to run in this proc and not in another one.
    // runTest(testInput, false);
    ASSERT_EXIT({ runTest(testInput); }, ::testing::ExitedWithCode(0), "Success"); // NOLINT
}

TEST_F(TestLoggerConfig, GlobalInfoLogWrittenToFile) // NOLINT
{
    TestInput testInput{ .logConfig = R"(
[global]
VERBOSITY=INFO
)",
                         .productContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN,
                         .productDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT,
                         .moduleContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN,
                         .moduleDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT,
                         .rootPath = TestLoggerConfig::testRunPath->dirPath() };
    EXPECT_EXIT({ runTest(testInput); }, ::testing::ExitedWithCode(0), "Success"); // NOLINT
}

TEST_F(TestLoggerConfig, TargetProductAndModuleDifferently) // NOLINT
{
    TestInput testInput{ .logConfig = R"(
[testlogging]
VERBOSITY=WARN
[module]
VERBOSITY=DEBUG
)",
                         .productContainLogs = LogLevels::ERROR | LogLevels::WARN,
                         .productDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT,
                         .moduleContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN | LogLevels::DEBUG |
                                              LogLevels::SUPPORT,
                         .moduleDoesNotContainLogs = 0,
                         .rootPath = TestLoggerConfig::testRunPath->dirPath() };
    EXPECT_EXIT({ runTest(testInput); }, ::testing::ExitedWithCode(0), "Success"); // NOLINT
}

TEST_F(TestLoggerConfig, TargetProductAndModuleWithReducingLevelForModule) // NOLINT
{
    TestInput testInput{ .logConfig = R"(
[testlogging]
VERBOSITY=DEBUG
[module]
VERBOSITY=WARN
)",
                         .productContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN | LogLevels::DEBUG |
                                               LogLevels::SUPPORT,
                         .productDoesNotContainLogs = 0,
                         .moduleContainLogs = LogLevels::ERROR | LogLevels::WARN,
                         .moduleDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT,
                         .rootPath = TestLoggerConfig::testRunPath->dirPath() };
    EXPECT_EXIT({ runTest(testInput); }, ::testing::ExitedWithCode(0), "Success"); // NOLINT
}

TEST_F(TestLoggerConfig, ConfigMayContainCommentSpacesAndAnyOtherNonRelatedEntries) // NOLINT
{
    TestInput testInput{ .logConfig = R"(
# this is the config file for sophos
# valid options are VERBOSITY=WARN
[testlogging]
VERBOSITY=INFO

# you can choose to target any module
[module]

VERBOSITY = WARN
ANOTHERENTRY = anything
)",
                         .productContainLogs = LogLevels::INFO | LogLevels::ERROR | LogLevels::WARN,
                         .productDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT,
                         .moduleContainLogs = LogLevels::ERROR | LogLevels::WARN,
                         .moduleDoesNotContainLogs = LogLevels::DEBUG | LogLevels::SUPPORT | LogLevels::INFO,
                         .rootPath = TestLoggerConfig::testRunPath->dirPath() };
    EXPECT_EXIT({ runTest(testInput); }, ::testing::ExitedWithCode(0), "Success"); // NOLINT
}

#endif