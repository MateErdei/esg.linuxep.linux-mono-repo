/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <boost/thread/thread.hpp>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/SplitProcesses.h>
#include <tests/Common/Helpers/TempDir.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>

bool skipTest()
{
#ifndef NDEBUG
    std::cout << "not compatible with coverage ... skip" << std::endl;
    return true;
#else
    if (getuid() != 0)
    {
        std::cout << "requires root ... skip" << std::endl;
        return true;
    }
    return false;
#endif

}

#define MAYSKIP if(skipTest()) return

using namespace CommsComponent;


class TestSplitProcesses : public ::testing::Test
{

public:
    TestSplitProcesses() : m_rootPath("/tmp"),m_tempDir{m_rootPath,"TestSplitProcesses"}
    {
        testing::FLAGS_gtest_death_test_style = "threadsafe";

    }

    UserConf m_lowPrivChildUser = {"lp", "lp"};
    UserConf m_lowPrivParentUser = {"games", "games"};
    std::string m_logConfigPath = {"base/etc/logger.conf"};
    std::string m_rootPath;
    std::string m_chrootDir;
    Tests::TempDir m_tempDir;
    std::string m_chrootSophosInstall;


    void SetUp()
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempDir.dirPath());
        std::string content = R"(
[global]
VERBOSITY=DEBUG
)";
        m_tempDir.makeTempDir();
        auto fperms = Common::FileSystem::FilePermissionsImpl();
        fperms.chmod(m_tempDir.dirPath(), 0777);

        m_tempDir.createFile(m_logConfigPath, content);
        m_tempDir.makeDirs(std::vector<std::string>{"var/sophos-spl-comms/logs/base","var/sophos-spl-comms/base/etc", "logs/base/sophosspl"});
        

        std::string sophosInstall = m_tempDir.dirPath();
        m_chrootDir = m_tempDir.absPath("var");
        m_chrootSophosInstall = m_tempDir.absPath("var/sophos-spl-comms");

        //network user dirs permissions to be done by the installer
        for(auto path : std::vector<std::string>{"logs","logs/base","logs/base/sophosspl","base", "base/etc/", "base/etc/logger.conf"}){
            fperms.chmod(Common::FileSystem::join(sophosInstall, path), 0777);    
        }
        fperms.chmod(m_chrootDir, 0777);

        for(auto path : std::vector<std::string>{"logs", "base", "base/etc/"}){
             fperms.chmod(Common::FileSystem::join(m_chrootSophosInstall, path), 0777);    
        }

        fperms.chmod(m_chrootSophosInstall, 0777);       
        
    }
};

class CommNetworkSide
{

public:
    // user can add any other method or constructors...

    // this becomes the 'main' function of the CommNetworkSide,
    // it can create threads, do whatever the business logic of that process is required.
    void operator()(std::shared_ptr<MessageChannel> channel, OtherSideApi &parentProxy)
    {
        while (true)
        {
            std::string message;
            channel->pop(message);
            std::cout << "child received: " << message << std::endl;
            if (message == "abort")
            {
                throw std::runtime_error("unexpected");
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
                return;
            }
            else if (message == "hello")
            {
                message = "world";
            }
            parentProxy.pushMessage(message + " fromchild");
        }
    }

};

class TestSplitProcessesWithNullConfigurator : public ::testing::Test {};
TEST_F(TestSplitProcessesWithNullConfigurator, ExchangeMessagesAndStop) // NOLINT
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                auto childProcess = CommNetworkSide();
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
                    childProxy.pushMessage("hello");
                    std::string message;
                    channel->pop(message);
                    if (message != "world fromchild")
                    {
                        std::cout << "did not receive the expected message" << message << std::endl; 
                        throw std::runtime_error("Did not receive world");
                    }
                    childProxy.pushMessage("stop");
                };
                int exitCode = splitProcessesReactors(parentProcess, childProcess);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");

}





TEST_F(TestSplitProcesses, ExchangeMessagesAndStop) // NOLINT
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    MAYSKIP;
    ASSERT_EXIT(
            {
                auto childProcess = CommNetworkSide();
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
                    childProxy.pushMessage("hello");
                    std::string message;
                    channel->pop(message);
                    if (message != "world fromchild")
                    {
                        throw std::runtime_error("Did not receive world");
                    }
                    childProxy.pushMessage("stop");
                };

                auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");

}

TEST_F(TestSplitProcesses, ParentIsNotifiedOnChildExit) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
                    childProxy.pushMessage("stop");
                    std::string message;
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException &)
                    {
                        return;
                    }
                    throw std::runtime_error("Did not receive closed channel exception");
                };
                auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}

TEST_F(TestSplitProcesses, ParentIsNotifiedIfChildAbort) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
                    childProxy.pushMessage("abort");
                    std::string message;
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException &)
                    {
                        return;
                    }
                    throw std::runtime_error("Did not receive closed channel exception");
                };
                auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}


// //Test that child can receive more than one message
// TEST_F(TestSplitProcesses, ChildCanRecieveMoreThanOneMessage) // NOLINT
// {
//     MAYSKIP;
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     ASSERT_EXIT(
//             {
//                 auto childProcess = CommNetworkSide();

//                 auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
//                     auto message1 = std::async(std::launch::async, [&channel, &childProxy]() {
//                         childProxy.pushMessage("getuid");
//                         std::string message;
//                         channel->pop(message, std::chrono::milliseconds(200));
//                         assert(!message.empty());
//                     });
//                     auto message2 = std::async(std::launch::async, [&channel, &childProxy]() {
//                         childProxy.pushMessage("hello");
//                         std::string message;
//                         channel->pop(message, std::chrono::milliseconds(200));
//                         assert(!message.empty());
//                     });
//                     auto message3 = std::async(std::launch::async, [&channel, &childProxy]() {
//                         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//                         childProxy.pushMessage("stop");
//                         std::string message;
//                         channel->pop(message);
//                     });
//                     message1.get();
//                     message2.get();
//                     message3.get();
//                     return;
//                 };

//                 auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
//                 int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
//                 exit(exitCode);
//             },
//             ::testing::ExitedWithCode(0), ".*");
// }

// // Test that if child 'crashs (send abort)' the parent will detect and the split process will 'return'
// TEST_F(TestSplitProcesses, ParentWillReturnIfChildCrashes) // NOLINT
// {
//     MAYSKIP;
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     ASSERT_EXIT(
//             {
//                 auto childProcess = CommNetworkSide();


//                 auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {

//                     auto message1 = std::async(std::launch::async, [&channel, &childProxy]() {
//                         childProxy.pushMessage("getuid");
//                         std::string message;
//                         channel->pop(message);
//                         assert(!message.empty());
//                     });
//                     auto message2 = std::async(std::launch::async, [&channel, &childProxy]() {
//                         std::this_thread::sleep_for(std::chrono::milliseconds(300));
//                         childProxy.pushMessage("abort");
//                         std::string message;
//                         channel->pop(message);
//                         assert(!message.empty());
//                     });
//                     message1.get();
//                     message2.get();
//                 };

//                 auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
//                 int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
//                 exit(exitCode);
//             },
//             ::testing::ExitedWithCode(0), ".*");
// }

// // Test that if the child just exits (send stop) the parent will detect .. (no hanging)
// TEST_F(TestSplitProcesses, ParentStopIfChildSendStopNoHanging) // NOLINT
// {
//     MAYSKIP;
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     ASSERT_EXIT(
//             {
//                 auto childProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi & /*parentProxy*/) {
//                     std::string message;
//                     auto received_all = std::async(std::launch::async, [&channel]() {
//                         channel->pushStop();
//                     });
//                     received_all.get();
//                 };

//                 auto parentProcess = CommNetworkSide();


//                 auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
//                 int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
//                 exit(exitCode);
//             },
//             ::testing::ExitedWithCode(0), ".*");
// }

// // If the parent goes away the child finishes. FIXME blocks forever
// TEST_F(TestSplitProcesses, ChildWillReturnIfParentCrashes) // NOLINT
// {
//     MAYSKIP;
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     ASSERT_EXIT({
//                     auto parentProcess = [](std::shared_ptr<MessageChannel> /*channel*/,
//                                             OtherSideApi & /*childProxy*/) {
//                         sleep(1);
//                         abort();
//                     };

//                     auto childProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &parentProxy) {
//                         parentProxy.pushMessage("hello");
//                         std::string message;
//                         //pop without timeout blocks forever
//                         channel->pop(message);
//                     };

//                     auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
//                     int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
//                     exit(exitCode);
//                 },
//                 ::testing::ExitedWithCode(0), ".*");
// }


// TEST_F(TestSplitProcesses, TestLogging) // NOLINT
// {
//     MAYSKIP;
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     ASSERT_EXIT({
//                     auto parentProcess = [](std::shared_ptr<MessageChannel> /*channel*/,
//                                             OtherSideApi & /*childProxy*/) {};

//                     auto childProcess = [](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi & /*parentProxy*/) {
//                         LOGERROR("CHILD");
//                     };
//                     auto config = CommsConfigurator(m_chrootDir, m_lowPrivChildUser, m_lowPrivParentUser);
//                     int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
//                     exit(exitCode);
//                 },
//                 ::testing::ExitedWithCode(0), ".*");
// }