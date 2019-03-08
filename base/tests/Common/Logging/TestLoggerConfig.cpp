/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <tests/Common/Helpers/TempDir.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Common/Logging/SophosLoggerMacros.h>

#include <iostream>
#include <thread>

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(logger, x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(logger, x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(logger, x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(logger, x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(logger, x)  // NOLINT


class TestLoggerConfig : public ::testing::Test
{
public:
    TestLoggerConfig() : m_logConfigPath{"base/etc/logger.conf"}, tempDir("/tmp", "testlog")
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    }

    void setLogConfig( const std::string & content)
    {
        tempDir.createFile(m_logConfigPath, content);
    }

    void runTest( const std::string & productName, const std::string & componentName)
    {
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


    std::string getLogContent( const std::string & productName)
    {
        std::string logspath = Common::FileSystem::join("logs/base", productName + ".log");
        std::string content;
        int count = 0;
        do{
            count++;
            try
            {
                 content = tempDir.fileContent(logspath);
                if (content.find("TESTEND") != std::string::npos)
                {
                    return content;
                }
            }catch(std::exception & )
            {

            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }while( count < 10 );
        return content;
    }
    enum LogLevels : int{DEBUG=0x01, INFO=0x02, SUPPORT=0x04, WARN=0x08, ERROR=0x10};


    std::string m_logConfigPath;
    Tests::TempDir tempDir;

    bool productShould( const std::string & content, int containLogs, int notContainLogs)
    {
        bool contain = logShouldContain(content, "productname", containLogs);
        bool notContain = logShouldNotContain(content, "productname", notContainLogs);
        return contain && notContain;
    }

    bool moduleShould( const std::string & content, int containLogs, int notContainLogs)
    {
        bool contain = logShouldContain(content, "componentname", containLogs);
        bool notContain = logShouldNotContain(content, "componentname", notContainLogs);
        return contain && notContain;
    }


private:

    bool logShouldContain(const std::string & content, const std::string & logname, int logLevels)
    {
        bool passAll = true;
        if ( logLevels & LogLevels::DEBUG)
        {
            passAll = passAll && contains(content, logname + " debug");
        }
        if ( logLevels & LogLevels::INFO)
        {
            passAll = passAll && contains(content, logname + " info");
        }
        if ( logLevels & LogLevels::SUPPORT)
        {
            passAll = passAll && contains(content, logname + " support");
        }
        if ( logLevels & LogLevels::WARN)
        {
            passAll = passAll && contains(content, logname + " warn");
        }
        if ( logLevels & LogLevels::ERROR)
        {
            passAll = passAll && contains(content, logname + " error");
        }

        return passAll;

    }


    bool logShouldNotContain(const std::string & content, const std::string & logname, int logLevels)
    {
        bool passAll = true;
        if ( logLevels & LogLevels::DEBUG)
        {
            passAll = passAll && !contains(content, logname + " debug");
        }
        if ( logLevels & LogLevels::INFO)
        {
            passAll = passAll && !contains(content, logname + " info");
        }
        if ( logLevels & LogLevels::SUPPORT)
        {
            passAll = passAll && !contains(content, logname + " support");
        }
        if ( logLevels & LogLevels::WARN)
        {
            passAll = passAll && !contains(content, logname + " warn");
        }
        if ( logLevels & LogLevels::ERROR)
        {
            passAll = passAll && !contains(content, logname + " error");
        }

        return passAll;

    }


    bool contains( const std::string & content, const std::string & needle )
    {
        bool v =  content.find(needle) != std::string::npos;
        return v;
    }
};


TEST_F(TestLoggerConfig, GlobalSupportLogWrittenToFile) // NOLINT
{
    std::string productName{"testlogging"};
    std::string module{"module"};
    setLogConfig(R"(
[global]
VERBOSITY=SUPPORT
)");
    runTest(productName, module);
    std::string content = getLogContent(productName);

    EXPECT_TRUE(productShould(content, LogLevels::INFO|LogLevels::ERROR|LogLevels::WARN|LogLevels::SUPPORT, LogLevels::DEBUG)) << content;
    EXPECT_TRUE(moduleShould(content, LogLevels::INFO|LogLevels::ERROR|LogLevels::WARN|LogLevels::SUPPORT, LogLevels::DEBUG)) << content;
}

TEST_F(TestLoggerConfig, GlobalInfoLogWrittenToFile) // NOLINT
{
    std::string productName{"testlogging"};
    std::string module{"module"};
    setLogConfig(R"(
[global]
VERBOSITY=INFO
)");
    runTest(productName, module);
    std::string content = getLogContent(productName);

    EXPECT_TRUE(productShould(content, LogLevels::INFO|LogLevels::ERROR|LogLevels::WARN, LogLevels::DEBUG|LogLevels::SUPPORT)) << content;
    EXPECT_TRUE(moduleShould(content, LogLevels::INFO|LogLevels::ERROR|LogLevels::WARN, LogLevels::DEBUG|LogLevels::SUPPORT)) << content;
}

