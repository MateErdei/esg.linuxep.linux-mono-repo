/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessInfo.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <fstream>


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
        EXPECT_EQ(process->exitCode(), 0);
    }

    TEST(ProcessImpl, waitUntilProcessEndsShouldReturnTheCorrectCode) // NOLINT
    {
        auto process = createProcess();
        process->exec("/bin/sleep", { "0.1" });
        process->waitUntilProcessEnds();
        ASSERT_EQ(process->output(), "");
        EXPECT_EQ(process->exitCode(), 0);
    }

    TEST(ProcessImpl, getStatusWillDetectProcessFinished) // NOLINT
    {
        auto process = createProcess();
        process->exec("/bin/sleep", { "0.1" });
        bool detected = false;
        for(int i=0; i<15;i++)
        {
            if (process->getStatus()==Common::Process::ProcessStatus::FINISHED)
            {
                detected = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        EXPECT_TRUE(detected) << "Failed to detect process finished";
        ASSERT_EQ(process->output(), "");
        EXPECT_EQ(process->exitCode(), 0);
    }


    int asyncSleep()
    {
        auto process = createProcess();
        process->exec("/bin/sleep", { "0.1" });
        process->waitUntilProcessEnds();
        if (process->output() != "")
        {
            return 1;
        }
        if (process->exitCode(), 0)
        {
            return 2;
        }
        return 0;
    }

    void myTest()
    {
        auto process = createProcess();
        auto currPid = ::getpid();
        std::cout << "Test started for tid: " << std::this_thread::get_id() << std::endl;
        for (int i = 0; i < 3; i++)
        {
            std::cout << "sleep: " << std::this_thread::get_id() << " " << i << std::endl;
            process->exec("/bin/sleep", { "0.1" });
            ASSERT_NE(currPid, process->childPid());
            auto v1 = ::time(nullptr);

            while (process->wait(std::chrono::milliseconds{ 1 }, 0) != ProcessStatus::FINISHED)
                ;

            auto v2 = ::time(nullptr);

            EXPECT_EQ(process->output(), "");
            EXPECT_EQ(process->exitCode(), 0);

            EXPECT_TRUE(v2 - v1 <= 1) << "time elapsed: " << v2 - v1;
            std::cout << "Test: " << i << " finished: " << std::endl;
        }
    }

    TEST(ProcessImpl, TestDataRaceForProcessImpl)
    {
        Common::Logging::ConsoleLoggingSetup loggingSetup;
        auto f1 = std::async(std::launch::async, myTest);
        auto f2 = std::async(std::launch::async, myTest);
        f1.get();
        f2.get();
    }

    TEST(ProcessImpl, BugTwoProcessesShouldNotDeadlock) // NOLINT
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
        auto job1 = std::async(std::launch::async, asyncSleep);
        auto job2 = std::async(std::launch::async, asyncSleep);
        EXPECT_EQ(job2.get(), 0);
        EXPECT_EQ(job1.get(), 0);
    }

    TEST(ProcessImpl, TwoProcessesWaitingInDifferentlyShouldNotDeadlock) // NOLINT
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
        auto job1 = std::async(std::launch::async, asyncSleep);
        auto process = createProcess();
        process->exec("/bin/sleep", { "2" });
        process->wait(std::chrono::milliseconds{ 1 }, 10000);
        process->output();
        process->exitCode();
        EXPECT_EQ(job1.get(), 0);
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

    TEST(ProcessImpl, WaitFromDifferentThreadsDoesNotDeadLock) // NOLINT
    {
        Common::Logging::ConsoleLoggingSetup consoleLogging;
        auto process = createProcess();
        process->exec("/bin/sleep", { "5" });
        auto t2 = std::async(std::launch::async, [&process](){
            // I'm trying to demonstrate that calling wait after another thread had called output should not make it
            // wait for more than the timeToWait ( in this case 10 milliseconds).
            // 5 times this 10 milliseconds is still far less than the full time of this executable.
            // Calling wait after output has returned, will have the ProcessStatus::FINISHED as return value.
            for(int i=0; i<5; i++)
            {
                EXPECT_EQ(process->wait(std::chrono::milliseconds(10),1), Common::Process::ProcessStatus::TIMEOUT) << "iteration: " << i;
            }
            // Intention of the test already proved. Killing the process to finish as fast as possible.
            process->kill();
        });
        process->output();
        t2.get();
    }


    TEST(ProcessImpl, ProcessWillNotBlockOnGettingOutputAfterWaitUntillProcessEnds) // NOLINT
    {
        std::string bashScript = R"(#!/bin/bash
echo 'started'
>&2 echo {1..300}
echo 'keep running'
sleep 10000
)";
        Tests::TempDir tempdir;
        tempdir.createFile("script", bashScript);
        Tests::TestExecutionSynchronizer testExecutionSynchronizer;
        bool first = true;
        auto process = createProcess();
        process->setOutputLimit(100);
        std::string captureout;
        process->setOutputTrimmedCallback([&captureout, &testExecutionSynchronizer, &first](std::string out) {
            captureout += out;
            if (first)
            {
                first = false;
                testExecutionSynchronizer.notify();
            }
        });
        process->exec("/bin/bash", { tempdir.absPath("script") });
        int pid = process->childPid();
        auto fut = std::async(std::launch::async, [&pid, &testExecutionSynchronizer]() {
            testExecutionSynchronizer.waitfor(500);
            std::cout << "send sigterm " << std::endl;
            ::kill(pid, SIGTERM);
        });
        std::cout << "wait untill process ends " << std::endl;
        process->waitUntilProcessEnds();
        std::string out = process->output();
        EXPECT_THAT(out, ::testing::Not(::testing::HasSubstr("started")));
        // the time to kill and the ammount of bytes written to output are not 'deterministic', hence, only requiring
        // not empty.
        EXPECT_FALSE(out.empty());
        EXPECT_THAT(captureout, ::testing::HasSubstr("started"));
        fut.get();
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

    TEST(ProcessImpl, CanCaptureTrimmedOutput) // NOLINT
    {
        std::string captureout;
        auto process = createProcess();
        process->setOutputTrimmedCallback([&captureout](std::string out) { captureout += out; });
        process->setOutputLimit(100);
        std::stringstream input;
        for (int i = 0; i < 1000; i++)
        {
            input << i << " ";
        }
        std::string echoinput = input.str();
        process->exec("/bin/echo", { "-n", echoinput });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        std::string output = process->output();
        EXPECT_LE(output.size(), 200);
        EXPECT_GE(output.size(), 100);
        EXPECT_EQ(captureout + output, echoinput);
    }

    TEST(ProcessImpl, SimulationOfDataRace) // NOLINT
    {
        Common::Logging::ConsoleLoggingSetup loggingSetup;
        std::string bashScript = R"(#!/bin/bash
echo 'started'
echo {1..300}
echo 'keep running'
sleep 1
)";
        Tests::TempDir tempdir;
        tempdir.createFile("script", bashScript);
        auto process = createProcess();
        process->setOutputLimit(100);

        auto startScript = std::async(std::launch::async, [&process, &tempdir]() {
            for (int i = 0; i < 2; i++)
            {
                process->exec("/bin/bash", { tempdir.absPath("script") });
                process->wait(std::chrono::milliseconds(3), 1);
            }
        });

        auto getStatus = std::async(std::launch::async, [&process]() {
            for (int i = 0; i < 5000; i++)
            {
                process->getStatus();
                std::this_thread::sleep_for(std::chrono::microseconds(300));
            }
        });

        auto fastWait = std::async(std::launch::async, [&process]() {
            for (int i = 0; i < 5000; i++)
            {
                process->wait(std::chrono::milliseconds(1), 0);
                std::this_thread::sleep_for(std::chrono::microseconds(300));
            }
        });
        startScript.get();
        getStatus.get();
        fastWait.get();
    }


    TEST(ProcessImpl, CheckDataRaceAndThreadSafetyOnPublicInterfaceOfProcess) // NOLINT
    {

        std::string bashScript = R"(#!/bin/bash
echo 'started'
echo 'keep running'
sleep 1
)";
        Tests::TempDir tempdir;
        tempdir.createFile("script", bashScript);
        auto process = createProcess();
        process->setOutputLimit(100);
        std::atomic<bool> keeprunning{true};

        auto killThread = std::async(std::launch::async, [&keeprunning,&process]() {
                                while (keeprunning) {
                                    process->kill();
                                }
                            }
        );

        auto waitThread = std::async(std::launch::async, [&keeprunning,&process]() {
                                         while (keeprunning) {
                                             process->wait(std::chrono::milliseconds(3),2);
                                         }
                                     }
        );

        auto waitAllThread = std::async(std::launch::async, [&keeprunning,&process]() {
                                         while (keeprunning) {
                                             process->waitUntilProcessEnds();
                                         }
                                     }
        );

        auto statusThread = std::async(std::launch::async, [&keeprunning,&process]() {
                                            while (keeprunning) {
                                                process->getStatus();
                                            }
                                        }
        );

        for(int i=0; i<10;i++)
        {
            process->exec("/bin/bash", { tempdir.absPath("script") });
            process->waitUntilProcessEnds();
            process->output();
            process->exitCode();
        }
        keeprunning=false;
        killThread.get();
        waitThread.get();
        waitAllThread.get();
        statusThread.get();

    }


    std::string fileDescriptorsOfPid(int pid)
    {
        auto process = createProcess();
        std::string path = "/proc/" + std::to_string(pid) + "/fd";
        process->exec("/bin/ls", {"-l", path});
        return process->output();
    }

    TEST(ProcessImpl, ChildShouldNotKeepFileDescriptorsOfParent) // NOLINT
    {
        auto process = createProcess();
        std::ofstream ofs ("testkeepfiledesc.txt", std::ofstream::out);
        std::string parentFds = fileDescriptorsOfPid(::getpid());
        process->exec("/bin/sleep", { "5" });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::string childFds = fileDescriptorsOfPid(process->childPid());

        EXPECT_THAT(childFds, ::testing::Not(::testing::HasSubstr("testkeepfiledesc")));
        EXPECT_THAT(parentFds, ::testing::HasSubstr("testkeepfiledesc"));
        process->kill(0);
    }


    TEST(ProcessImpl, DoesNotUseShellToExecute)
    {
        std::string content = R"(This is not an executable)";
        Tests::TempDir tempdir;
        tempdir.createFile("script", content);
        auto filePath = tempdir.absPath("script");
        // make it executable
        int ret= ::chmod(filePath.c_str(), 0700);
        ASSERT_EQ(ret, 0);
        auto process = createProcess();
        process->exec(filePath, {});
        std::string out = process->output();
        int exitCode = process->exitCode();
        ASSERT_EQ(exitCode, ENOEXEC);
        EXPECT_THAT(out, ::testing::Not(::testing::HasSubstr("This: not found")));
    }


} // namespace
