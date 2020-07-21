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

bool skipIfCoverage()
{
// I couldnt' find a way to detect that coverage is enabled (gcov), hence assuming debug is the same as coverage
#ifndef NDEBUG
    std::cout << "not compatible with coverage ... skip" << std::endl;
    return true;
#else
    return false;
#endif
}

#define SKIPIFCOVERAGE  if (skipIfCoverage()) return

#define MAYSKIP if(skipTest())  return


using namespace CommsComponent;


class TestSplitProcesses : public ::testing::Test
{

public:
    TestSplitProcesses() : m_rootPath("/tmp"), m_tempDir{m_rootPath, "TestSplitProcesses"}
    {
        testing::FLAGS_gtest_death_test_style = "threadsafe";
    }

    UserConf m_lowPrivChildUser = {"lp", "lp", "child"};
    UserConf m_lowPrivParentUser = {"games", "lp", "parent"};
    std::string m_logConfigPath = {"base/etc/logger.conf"};
    std::string m_rootPath;
    Tests::TempDir m_tempDir;
    std::string m_chrootSophosInstall;


    void setupAfterSkipIfNotRoot()
    {
        std::cout << "called setupAfterSkipIfNotRoot" << std::endl;
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
        m_tempDir.makeDirs(std::vector<std::string>{
                "var/sophos-spl-comms/logs/base", "var/sophos-spl-comms/base/etc", "logs/base/sophosspl"
        });


        std::string sophosInstall = m_tempDir.dirPath();
        m_chrootSophosInstall =  CommsConfigurator::chrootPathForSSPL(sophosInstall);

        //local user dirs permissions to be done by the installer
        for (auto path : std::vector<std::string>{
                "logs", "logs/base", "logs/base/sophosspl", "base", "base/etc/", "base/etc/logger.conf"
        })
        {
            fperms.chmod(Common::FileSystem::join(sophosInstall, path), 0777);
        }
        //network user dirs permissions to be done by the installer
        fperms.chmod(m_tempDir.absPath("var"), 0777);
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
    void operator()(std::shared_ptr<MessageChannel> channel, OtherSideApi &parentProxy)
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
                setupAfterSkipIfNotRoot();
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
                auto childProcess = [](std::shared_ptr<MessageChannel> , OtherSideApi &)
                {
                    exit(3); 
                };
                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &) {
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

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
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

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
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

                auto parentProcess = [](std::shared_ptr<MessageChannel> channel, OtherSideApi &childProxy) {
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

TEST_F(TestSplitProcesses, ParentAndChildShouldBeAbleToUseLog4) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    std::string captureTempPath;
    ASSERT_EXIT({
                    setupAfterSkipIfNotRoot();
                    captureTempPath = m_tempDir.dirPath();
                    auto parentProcess = [](std::shared_ptr<MessageChannel> /*channel*/,
                                            OtherSideApi & /*childProxy*/) {
                        LOGDEBUG("Log from Parent");

                    };

                    auto childProcess = [](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi & /*parentProxy*/) {
                        LOGDEBUG("Log from Child");
                        std::cout << "also log from here" << std::endl;
                        try
                        {
                            Common::FileSystem::fileSystem()->writeFile("/logs/base/another_child.log", "testing123");
                        }
                        catch (std::exception& ex)
                        {
                            std::cerr << "failed to write file: " << ex.what() << std::endl;
                        }
                    };
                    auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                    std::vector<ReadOnlyMount>());
                    int exitCode = splitProcessesReactors(parentProcess, childProcess, config);


                    std::string childLog = Common::FileSystem::join(m_chrootSophosInstall,
                                                                    "logs/base/child.log");
                    std::string parentLog = m_tempDir.absPath("logs/base/sophosspl/parent.log");
                    auto fs = Common::FileSystem::fileSystem();
                    std::string parentLogContent = fs->readFile(parentLog);
                    if (parentLogContent.find("Log from Parent") == std::string::npos)
                    {
                        exitCode = 2;
                    }
                    for (auto &name : fs->listFilesAndDirectories(
                            Common::FileSystem::join(m_chrootSophosInstall, "logs/base/")))
                    {
                        std::cout << "files inside the child log base: " << name << std::endl;
                    }
                    std::string childLogContent = fs->readFile(childLog);
                    if (childLogContent.find("Log from Child") == std::string::npos)
                    {
                        exitCode = 3;
                    }

                    exit(exitCode);
                },
                ::testing::ExitedWithCode(0), ".*");
    std::cout << "temp path " << captureTempPath << std::endl;
}

TEST_F(TestSplitProcesses, ChildCanMountDependedDirectories) // NOLINT
{
    //MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    std::string captureTempPath;
    ASSERT_EXIT({
                    setupAfterSkipIfNotRoot();
                    captureTempPath = m_tempDir.dirPath();
                    std::cout << captureTempPath << std::endl;

                    m_tempDir.makeDirs("sourceDir");
                    auto sourceDir = m_tempDir.absPath("sourceDir");
                    auto mountedDirContentFile = Common::FileSystem::join(sourceDir, "testMountDir.txt");
                    Common::FileSystem::fileSystem()->writeFile(mountedDirContentFile, "FileFromMountedDir");

                    //open permissions and prove only readonly mount prevent changing
                    Common::FileSystem::FilePermissionsImpl().chmod(sourceDir, 0777);
                    Common::FileSystem::FilePermissionsImpl().chmod(mountedDirContentFile, 0666);
                    std::string mountedDirRelativePath = "targetDir";

                    //source path and relative path after chroot directory
                    auto dirDep = std::make_pair(sourceDir, mountedDirRelativePath);

                    std::vector<ReadOnlyMount> mountDependencies;
                    mountDependencies.emplace_back(dirDep);

                    auto parentProcess = [](std::shared_ptr<MessageChannel> channel,
                                            OtherSideApi& /*childProxy*/) {
                        LOGDEBUG("Log from Parent");
                        std::string message;
                        sleep(100);
                        channel->pop(message);
                    };

                    auto childProcess = [&](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi& /*parentProxy*/) {
                        LOGDEBUG("Log from Child");

                        auto cfs = Common::FileSystem::fileSystem();
                        //verify file inside mounted directory is accessible
                        auto testMountPath = Common::FileSystem::join("/", mountedDirRelativePath, "testMountDir.txt");
                        std::cout << "try read " << testMountPath << std::endl;
                        sleep(100);
                        auto mountDirContent = cfs->readFile(testMountPath);
                        std::cout << "mountDirContent" << std::endl;
                        if (mountDirContent.find("FileFromMountedDir") == std::string::npos)
                        {
                            throw std::runtime_error("Failed to read file from mounted directoty");
                        }
                        //Mounted as read only file remove should fail
                        try
                        {
                            cfs->removeFile(Common::FileSystem::join("/", mountedDirRelativePath, "testMountDir.txt"));
                            throw std::runtime_error("remove file on readonly directory must fail");
                        }
                        catch (const Common::FileSystem::IFileSystemException& error)
                        {
                            std::cout << error.what() << std::endl;
                            std::cerr << "write failed as expected " << error.what() << std::endl;
                        }
                        return;

                    };
                    auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                    mountDependencies);
                    int exitCode = splitProcessesReactors(parentProcess, childProcess, config);

                    exit(exitCode);
                },
                ::testing::ExitedWithCode(0), ".*");
}

TEST_F(TestSplitProcesses, ChildCanMountDependedFilesReadOnly) // NOLINT
{
    MAYSKIP;
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    std::string captureTempPath;
    ASSERT_EXIT({
                    setupAfterSkipIfNotRoot();
                    captureTempPath = m_tempDir.dirPath();

                    std::string sourceFileName = "sourceFile.txt";
                    m_tempDir.makeDirs("sourceFileDir");
                    auto sourceFileDir = m_tempDir.absPath("sourceFileDir");
                    auto sourceFile = Common::FileSystem::join(sourceFileDir, sourceFileName);
                    Common::FileSystem::fileSystem()->writeFile(sourceFile, "MountedSourceFile");
                    Common::FileSystem::FilePermissionsImpl().chmod(sourceFile, 644);

                    std::string mountedFileRelativePath = Common::FileSystem::join("targetFileDir", sourceFileName);

                    //source path and relative path after chroot directory
                    auto dfileDep = std::make_pair(sourceFile, mountedFileRelativePath);
                    std::vector<ReadOnlyMount> mountDependencies;
                    mountDependencies.emplace_back(dfileDep);

                    auto parentProcess = [](std::shared_ptr<MessageChannel> /*channel*/,
                                            OtherSideApi& /*childProxy*/) {
                    };

                    auto childProcess = [&](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi& /*parentProxy*/) {

                        auto cfs = Common::FileSystem::fileSystem();
                        //verify file inside mounted directory is accessible
                        std::cout << "try read mount from Child" << std::endl;

                        auto mountFileContent = cfs->readFile(
                                Common::FileSystem::join("/", mountedFileRelativePath));
                        std::cout << "mounted content: " << mountFileContent << std::endl;
                        if (mountFileContent.find("MountedSourceFile") == std::string::npos)
                        {
                            throw std::runtime_error("Failed to read file from mounted directoty");
                        }
                        try
                        {
                            cfs->writeFile(Common::FileSystem::join("/", mountedFileRelativePath), "ovewrite");
                            throw std::runtime_error("Write fail should fail for read only mounted file");
                        }
                        catch (const Common::FileSystem::IFileSystemException& error)
                        {
                            std::cout << "write failed as expected " << error.what() << std::endl;
                            std::cerr << "write failed as expected " << error.what() << std::endl;
                        }
                        throw std::runtime_error("Passed");
                    };
                    auto config = CommsConfigurator(m_chrootSophosInstall, m_lowPrivChildUser, m_lowPrivParentUser,
                                                    mountDependencies);
                    int exitCode = splitProcessesReactors(parentProcess, childProcess, config);

                    exit(exitCode);
                },
                ::testing::ExitedWithCode(0), ".*");
}