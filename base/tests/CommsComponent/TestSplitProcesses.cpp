/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef ARTISANBUILD
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileSystem.h>
#include <boost/thread/thread.hpp>
#include <gtest/gtest.h>
#include <modules/CommsComponent/SplitProcesses.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <tests/Common/Helpers/TestMacros.h>
#include <tests/Common/Helpers/OutputToLog.h>
#include <fstream>
#include <iostream>

using namespace CommsComponent;

const char * TestSplitProc="/tmp/TestSplitProcesses";
class TestSplitProcesses : public ::testing::Test
{

public:
    static void SetUpTestCase()
    {
        Common::FileSystem::fileSystem()->makedirs(TestSplitProc);
        Common::FileSystem::filePermissions()->chmod(TestSplitProc, 0777);
    }

    static void TearDownTestCase()
    {
        Common::FileSystem::fileSystem()->removeFileOrDirectory(TestSplitProc);
    }

    TestSplitProcesses() : m_rootPath(TestSplitProc)
    {
        testing::FLAGS_gtest_death_test_style = "threadsafe";
    }

    UserConf m_lowPrivChildUser = {"lp", "lp", "child"};
    UserConf m_lowPrivParentUser = {"games", "lp", "parent"};

    std::string m_rootPath;
    std::string m_chrootSophosInstall;
    std::string m_sophosInstall;


    void setupAfterSkipIfNotRoot()
    {
        std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        m_sophosInstall = Common::FileSystem::join(m_rootPath, testName);

        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, m_sophosInstall);
        std::string content = R"(
[global]
VERBOSITY=DEBUG
)";
        auto fs = Common::FileSystem::fileSystem();
        auto fperms = Common::FileSystem::FilePermissionsImpl();
        fs->makedirs(m_sophosInstall);
        fperms.chmod(m_sophosInstall, 0777);

        //create local user dirs and set permissions already done by the installer
        for (auto path : std::vector<std::string>{
                "logs", "logs/base", "logs/base/sophosspl", "base", "base/etc/",
        })
        {
            auto absPath = Common::FileSystem::join(m_sophosInstall, path);
            fs->makedirs(absPath);
            fperms.chmod(absPath, 0777);
        }

        //create expected files and open permissions
        auto loggerConf = Common::FileSystem::join(m_sophosInstall, "base/etc/logger.conf");
        fs->writeFile(loggerConf, content);
        fperms.chmod(loggerConf, 0666);

        //network user dirs permissions to be done by the installer
        m_chrootSophosInstall = CommsConfigurator::chrootPathForSSPL(
                m_sophosInstall); //SOPHOS_INSTALL/var/sophos-spl-comms
        fs->makedirs(m_chrootSophosInstall);
        fperms.chmod(Common::FileSystem::join(m_sophosInstall, "var"), 0777);
        fperms.chmod(m_chrootSophosInstall, 0777);

        for (auto path : std::vector<std::string>{"logs", "base", "base/etc/", "logs/base"})
        {
            auto abdPath = Common::FileSystem::join(m_chrootSophosInstall, path);
            fs->makedirs(abdPath);
            fperms.chmod(abdPath, 0777);
        }
    }
};

class CommNetworkSide
{

public:
    // user can add any other method or constructors...

    // this becomes the 'main' function of the CommNetworkSide,
    // it can create threads, do whatever the business logic of that process is required.
    int operator()(std::shared_ptr<MessageChannel> channel, IOtherSideApi& parentProxy)
    {
        while (true)
        {
            std::string message;
            channel->pop(message);
            std::cout << "child received: " << message << std::endl;
            if (message == "exception")
            {
                throw std::runtime_error("unexpected");
            }
            if (message == "abort")
            {

                std::cout << "aborting " << std::endl;
                std::terminate();
            }
            if (message == "getuid")
            {
                // capture the current uid and respond
                std::stringstream guidmsg;
                guidmsg << "child guid: " << getuid();
                message = guidmsg.str();
            }
            else if (message == "stop")
            {
                break;
            }
            else if (message == "hello")
            {
                message = "world";
            }
            parentProxy.pushMessage(message + " fromchild");
        }
        return 0; 
    }

};

class TestSplitProcessesWithNullConfigurator : public ::testing::Test
{
};

TEST_F(TestSplitProcessesWithNullConfigurator, ExchangeMessagesAndStop) // NOLINT
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    SKIPIFCOVERAGE;
    ASSERT_EXIT(
            {
                auto childProcess = CommNetworkSide();
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi& childProxy) {
                    childProxy.pushMessage("hello");
                    std::string message;
                    channel->pop(message);
                    if (message != "world fromchild")
                    {
                        std::cout << "did not receive the expected message" << message << std::endl;
                        throw std::runtime_error("Did not receive world");
                    }
                    childProxy.pushMessage("stop");
                    return 0; 
                };
                int exitCode = splitProcessesReactors(parentProcess, childProcess);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");

}


TEST_F(TestSplitProcessesWithNullConfigurator, ParentExportsErrorCodeOfTheChild) // NOLINT
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    MAYSKIP;
    ASSERT_EXIT(
            {
                auto childProcess = [](std::shared_ptr<MessageChannel>, IOtherSideApi&) {
                    return 3; 
                };
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi&) {
                    std::string message;
                    channel->pop(message);
                    return 0; 
                };

                int exitCode = splitProcessesReactors(parentProcess, childProcess);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(3), ".*");

}


TEST_F(TestSplitProcesses, ParentIsNotifiedOnChildExit) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi& childProxy) {
                    childProxy.pushMessage("stop");
                    std::string message;
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException&)
                    {
                        return 0;
                    }
                    throw std::runtime_error("Did not receive closed channel exception");       
                    return 0;              
                };
                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}

TEST_F(TestSplitProcesses, ParentIsNotifiedIfChildAbort) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    //If this unit test has been fixed to become less flakey please remove the debug log lines
    //Tests::OutputToLog::writeToUnitTestLog(<log message>);
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi& childProxy) {
                    childProxy.pushMessage("abort");
                    std::string message;
                    Tests::OutputToLog::writeToUnitTestLog("TestSplitProcesses.ParentIsNotifiedIfChildAbort");
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException&)
                    {
                        Tests::OutputToLog::writeToUnitTestLog("Got ChannelClosedException");
                        return 0;
                    }
                    catch (std::exception& ex)
                    {
                        Tests::OutputToLog::writeToUnitTestLog(ex.what());
                    }

                    Tests::OutputToLog::writeToUnitTestLog("Did not receive closed channel exception, message: " + message);
                    throw std::runtime_error("Did not receive closed channel exception, message: " + message);
                    return 0; 
                };
                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);

                std::stringstream message;
                message << "Exitcode: " << exitCode;
                Tests::OutputToLog::writeToUnitTestLog(message.str());
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}


// //Test that child can receive more than one message
TEST_F(TestSplitProcesses, ChildCanRecieveMoreThanOneMessageAndConcurrently) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi& childProxy) {
                    auto message1 = std::async(std::launch::async, [&channel, &childProxy]() {
                        childProxy.pushMessage("getuid");
                        std::string message;
                        channel->pop(message, std::chrono::milliseconds(200));
                        assert(!message.empty());
                    });
                    auto message2 = std::async(std::launch::async, [&channel, &childProxy]() {
                        childProxy.pushMessage("hello");
                        std::string message;
                        channel->pop(message, std::chrono::milliseconds(200));
                        assert(!message.empty());
                    });
                    message1.get();
                    message2.get();
                    std::string message;
                    childProxy.pushMessage("stop");
                    channel->pop(message);
                    return 0;
                };

                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}

// // Test that if the child just exits (send stop) the parent will detect .. (no hanging)
TEST_F(TestSplitProcesses, ParentStopIfChildSendStopNoHanging) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi& /*parentProxy*/) {
                    channel->pushStop();
                    return 0; 
                };

                auto parentProcess = CommNetworkSide();

                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}
#endif