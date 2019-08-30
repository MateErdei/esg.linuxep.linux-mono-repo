/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessInfo.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <fstream>
#include <Common/FileSystem/IFileSystem.h>

using namespace Common::Process;
namespace
{
    void removeFile(const std::string& path)
    {
        int ret = ::remove(path.c_str());
        if (ret == -1 && errno == ENOENT)
        {
            return;
        }
        ASSERT_EQ(ret, 0);
    }

    TEST(ProcessImpl, SimpleEchoShouldReturnExpectedString) // NOLINT
    {
        auto process = createProcess();
        process->exec("/bin/echo", { "hello" });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST(ProcessImpl, SupportMultiplesArgs) // NOLINT
    {
        auto process = createProcess();
        process->exec("/bin/echo", { "hello", "world" });
        ASSERT_EQ(process->output(), "hello world\n");
    }

    TEST(ProcessImpl, OutputBlockForResult) // NOLINT
    {
        for (int i = 0; i < 10; i++)
        {
            auto process = createProcess();
            process->exec("/bin/echo", { "hello" });
            ASSERT_EQ(process->output(), "hello\n");
        }
    }

    TEST(ProcessImpl, WaitpidWaitsUntilProcessEndsSuccessfully) // NOLINT
    {
        auto process = createProcess();
        process->exec("/bin/echo", { "hello" });
        process->waitUntilProcessEnds();
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST(ProcessImpl, ProcessNotifyOnClosure) // NOLINT
    {
        auto process = createProcess();
        Tests::TestExecutionSynchronizer testExecutionSynchronizer;
        process->setNotifyProcessFinishedCallBack(
            [&testExecutionSynchronizer]() { testExecutionSynchronizer.notify(); });
        process->exec("/bin/echo", { "hello" });
        EXPECT_TRUE(testExecutionSynchronizer.waitfor());
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST(ProcessImpl, ProcessNotifyOnClosureShouldNotRequireUsageOfStandardOutput) // NOLINT
    {
        auto process = createProcess();
        Tests::TestExecutionSynchronizer testExecutionSynchronizer;
        process->setNotifyProcessFinishedCallBack(
            [&testExecutionSynchronizer]() { testExecutionSynchronizer.notify(); });
        process->exec("/bin/sleep", { "0.1" });
        EXPECT_TRUE(testExecutionSynchronizer.waitfor());
        ASSERT_EQ(process->output(), "");
    }

    TEST(ProcessImpl, ProcessNotifyOnClosureShouldAlsoWorkForInvalidProcess) // NOLINT
    {
        auto process = createProcess();
        Tests::TestExecutionSynchronizer testExecutionSynchronizer;
        process->setNotifyProcessFinishedCallBack(
            [&testExecutionSynchronizer]() { testExecutionSynchronizer.notify(); });
        // the process will not work correctly. But the notification on its failure should still be triggered.
        process->exec("/bin/nothingsleep", { "0.1" });
        EXPECT_TRUE(testExecutionSynchronizer.waitfor());
        ASSERT_EQ(process->output(), "");
    }

    TEST(ProcessImpl, SupportAddingEnvironmentVariables) // NOLINT
    {
        std::string testFilename("envTestGood.sh");

        removeFile(testFilename);
        std::fstream out(testFilename, std::ofstream::out);
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        process->exec("/bin/bash", { testFilename });
        EXPECT_EQ(process->output(), "is\n");

        process = createProcess();
        process->exec("/bin/bash", { testFilename }, { { "os", "linux" }, { "adj", "great" } });
        EXPECT_EQ(process->output(), "linux is great\n");
        removeFile(testFilename);
    }

    TEST(ProcessImpl, AddingInvalidEnvironmentVariableShouldFail) // NOLINT
    {
        std::string testFilename("envTestBad.sh");

        removeFile(testFilename);
        std::fstream out(testFilename, std::ofstream::out);
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        EXPECT_THROW( // NOLINT
            process->exec("/bin/bash", { testFilename }, { { "", "linux" }, { "adj", "great" } }),
            Common::Process::IProcessException); // NOLINT

        removeFile(testFilename);
    }

    TEST(ProcessImpl, TouchFileCommandShouldCreateFile) // NOLINT
    {
        auto process = createProcess();
        removeFile("success.txt");

        process->exec("/usr/bin/touch", { "success.txt" });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        EXPECT_EQ(process->exitCode(), 0);
        std::ifstream ifs2("success.txt", std::ifstream::in);
        EXPECT_TRUE(ifs2);
        removeFile("success.txt");
    }

    TEST(ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail) // NOLINT
    {
        auto process = createProcess();

        process->exec("/usr/bin/touch", { "" });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);

        auto error = process->output();
        EXPECT_THAT(error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ(process->exitCode(), 1);
    }

    TEST(ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail_WithoutCallingWait) // NOLINT
    {
        auto process = createProcess();
        process->exec("/usr/bin/touch", { "" });
        auto error = process->output();
        EXPECT_THAT(error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ(process->exitCode(), 1);
    }

    TEST(ProcessImpl, NonExistingCommandShouldFail) // NOLINT
    {
        Common::Logging::ConsoleLoggingSetup consoleLogging;
        auto process = createProcess();
        process->exec("/bin/command_does_not_exists", { "fake_argument" });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        EXPECT_EQ(process->output(), "");
        EXPECT_EQ(process->exitCode(), 2);
    }

    TEST(ProcessImpl, LongCommandExecutionShouldTimeout) // NOLINT
    {
        std::fstream out("test.sh", std::ofstream::out);
        out << "while true; do echo 'continue'; sleep 1; done\n";
        out.close();
        auto process = createProcess();
        process->exec("/bin/bash", { "test.sh" });
        EXPECT_EQ(process->wait(milli(200), 1), ProcessStatus::TIMEOUT);

        process->kill();

        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);

        auto output = process->output();

        EXPECT_THAT(output, ::testing::HasSubstr("continue"));
        removeFile("test.sh");
    }

    TEST(ProcessImpl, SupportOutputWithMultipleLines) // NOLINT
    {
        std::string targetfile = "/etc/passwd";

        std::ifstream t(targetfile, std::istream::in);
        std::string content((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        t.close();
        auto process = createProcess();
        process->exec("/bin/cat", { targetfile });
        ASSERT_EQ(process->output(), content);
    }

    TEST(ProcessImpl, OutputCannotBeCalledBeforeExec) // NOLINT
    {
        auto process = createProcess();
        EXPECT_THROW(process->output(), IProcessException); // NOLINT
    }

    TEST(ProcessImpl, ExitCodeCannotBeCalledBeforeExec) // NOLINT
    {
        auto process = createProcess();
        EXPECT_THROW(process->exitCode(), IProcessException); // NOLINT
    }

    TEST(ProcessImpl, TestCreateEmptyProcessInfoCreatesEmptyObject) // NOLINT
    {
        auto processInfoPtr = Common::Process::createEmptyProcessInfo();

        ASSERT_EQ(processInfoPtr->getExecutableFullPath(), "");
        std::vector<std::string> emptyVector;
        ASSERT_EQ(processInfoPtr->getExecutableArguments(), emptyVector);
        Common::Process::EnvPairs emptyEnvPairs;
        ASSERT_EQ(processInfoPtr->getExecutableEnvironmentVariables(), emptyEnvPairs);
        ASSERT_EQ(processInfoPtr->getExecutableUserAndGroupAsString(), "");

        auto groupPair = processInfoPtr->getExecutableGroup();
        ASSERT_EQ(groupPair.first, false);
        ASSERT_EQ(groupPair.second, -1);
        auto userPair = processInfoPtr->getExecutableUser();
        ASSERT_EQ(userPair.first, false);
        ASSERT_EQ(userPair.second, -1);
    }

namespace{
    std::string pythonScript = R"(
import signal
import time
import os
def handler(signum,frame):
 return
signal.signal(signal.SIGTERM,handler)
print ("armed signal")
print (os.getpid())
count=0
while count < 2:
    count += 1
    time.sleep(10)
)";

    std::string& pythonFullPath()
    {
        static std::vector<std::string> candidates = {
                {"/usr/bin/python"},
                {"/usr/bin/python3.6"},
                {"/usr/bin/python2.7"},
                {"/usr/bin/python2"},
                {"/usr/bin/python"}
        };
        for( auto & path: candidates)
        {
            if ( Common::FileSystem::fileSystem()->exists(path))
            {
                return path;
            }
        }
        throw std::runtime_error("Could not find path of python");
    }

}


    TEST(ProcessImpl, WaitShouldBeRespected)
    {
        auto proc = Common::Process::createProcess();
        proc->exec(pythonFullPath(), {"-c", pythonScript} );

        auto now = std::chrono::system_clock::now();

        // sending the kill too earlier may get to python before it has armed the signal handler.
        // this sleep is to try to avoid this.
        ASSERT_EQ( proc->wait(Common::Process::milli(300), 2), Common::Process::ProcessStatus::TIMEOUT) << proc->output();
        auto after = std::chrono::system_clock::now();
        auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(after-now).count();
        EXPECT_GE(passed, 600);
        EXPECT_LE(passed, 2000);
    }

} // namespace
