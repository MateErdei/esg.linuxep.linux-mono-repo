/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <boost/thread/thread.hpp>
#include <gtest/gtest.h>
#include <modules/CommsComponent/SplitProcesses.h>
#include <tests/Common/Helpers/TempDir.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <tests/Common/Helpers/TestMacros.h>

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
    std::string m_logConfigPath = {"base/etc/logger.conf"};
    std::string m_rootPath;
    std::unique_ptr<Tests::TempDir> m_tempDir;
    std::string m_chrootSophosInstall;


    void setupAfterSkipIfNotRoot()
    {
        m_tempDir.reset(new Tests::TempDir(m_rootPath, "TestSplitProcesses"));

        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempDir->dirPath());
        std::string content = R"(
[global]
VERBOSITY=DEBUG
)";
        auto fperms = Common::FileSystem::FilePermissionsImpl();
        fperms.chmod(m_tempDir->dirPath(), 0777);

        m_tempDir->createFile(m_logConfigPath, content);
        m_tempDir->makeDirs(std::vector<std::string>{
                "var/sophos-spl-comms/logs/base", "var/sophos-spl-comms/base/etc", "logs/base/sophosspl"
        });


        std::string sophosInstall = m_tempDir->dirPath();
        m_chrootSophosInstall = CommsConfigurator::chrootPathForSSPL(sophosInstall);

        //local user dirs permissions to be done by the installer
        for (auto path : std::vector<std::string>{
                "logs", "logs/base", "logs/base/sophosspl", "base", "base/etc/", "base/etc/logger.conf"
        })
        {
            fperms.chmod(Common::FileSystem::join(sophosInstall, path), 0777);
        }
        //network user dirs permissions to be done by the installer
        fperms.chmod(m_tempDir->absPath("var"), 0777);
        for (auto path : std::vector<std::string>{"logs", "base", "base/etc/", "logs/base"})
        {
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
    void operator()(std::shared_ptr<MessageChannel> channel, OtherSideApi& parentProxy)
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
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
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
                setupAfterSkipIfNotRoot();
                auto childProcess = CommNetworkSide();
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
                    childProxy.pushMessage("hello");
                    std::string message;
                    channel->pop(message);
                    if (message != "world fromchild")
                    {
                        throw std::runtime_error("Did not receive world");
                    }
                    childProxy.pushMessage("stop");
                };

                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");

}

TEST_F(TestSplitProcesses, ParentProcessExportTheErrorCodeOfTheChild) // NOLINT
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    MAYSKIP;
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = [](std::shared_ptr<MessageChannel>, OtherSideApi&) {
                    exit(3);
                };
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi&) {
                    std::string message;
                    channel->pop(message);
                };

                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
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

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
                    childProxy.pushMessage("stop");
                    std::string message;
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException&)
                    {
                        return;
                    }
                    throw std::runtime_error("Did not receive closed channel exception");
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
    ASSERT_EXIT(
            {
                setupAfterSkipIfNotRoot();
                auto childProcess = CommNetworkSide();

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
                    childProxy.pushMessage("abort");
                    std::string message;
                    try
                    {
                        channel->pop(message);
                    }
                    catch (ChannelClosedException&)
                    {
                        return;
                    }
                    throw std::runtime_error("Did not receive closed channel exception");
                };
                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
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

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
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
                    return;
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
                auto childProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& /*parentProxy*/) {
                    channel->pushStop();
                };

                auto parentProcess = CommNetworkSide();

                auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                std::vector<ReadOnlyMount>());
                int exitCode = splitProcessesReactors(parentProcess, childProcess, config);
                exit(exitCode);
            },
            ::testing::ExitedWithCode(0), ".*");
}