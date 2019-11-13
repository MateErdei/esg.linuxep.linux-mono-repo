/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#ifndef OutDir
#define OutDir ""
#endif
#ifndef ARTISANBUILD

using namespace Common::Process;
namespace
{

    std::string PickYourPoisonPath()
    {
        std::string OutputDir{OutDir};
        if( Common::FileSystem::fileSystem()->isDirectory(OutputDir))
        {
            std::string candidate = Common::FileSystem::join(OutputDir, "../../manualTools/PickYourPoison");
            if (Common::FileSystem::fileSystem()->isExecutable(candidate))
            {
                return candidate;
            }
        }
        throw std::runtime_error("Can not find PickYourPoison");
    }

    TEST(TestProcImplAndPyP, DISABLED_CrashShouldBeDetectedAndNotCrashProcess) // NOLINT
    {
        auto process = createProcess();
        process->exec(PickYourPoisonPath(), { "--crash" });
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::TIMEOUT);
        process->waitUntilProcessEnds();
        std::string output = process->output();
        EXPECT_THAT(output, ::testing::HasSubstr("using argument: --crash"));
        EXPECT_NE(process->exitCode(), 0);
    }

    // test passes, but it is slow... Disabling it.
    TEST(TestProcImplAndPyP, DISABLED_SpamShouldEventuallyNotCorrupt) // NOLINT
    {
        auto process = createProcess();
        process->setOutputLimit(100);
        process->exec(PickYourPoisonPath(), { "--spam" });
        EXPECT_EQ(process->wait(milli(10), 500), ProcessStatus::TIMEOUT);
        process->kill();
        std::string output = process->output();
        EXPECT_GT(output.size(), 100);
        EXPECT_LT(output.size(), 201);
        EXPECT_NE(process->exitCode(), 0);
    }
    // test passes, but it is slow... Disabling it.
    TEST(TestProcImplAndPyP, DISABLED_DeadlockAndKillShouldBeEnoughToRestore) // NOLINT
    {
        auto process = createProcess();
        process->setOutputLimit(100);
        process->exec(PickYourPoisonPath(), { "--deadlock" });
        EXPECT_EQ(process->wait(milli(10), 500), ProcessStatus::TIMEOUT);
        process->kill();
        std::string output = process->output();
        EXPECT_THAT(output, ::testing::HasSubstr("using argument: --deadlock"));
        EXPECT_NE(process->exitCode(), 0);
    }

    TEST(TestProcImplAndPyP, HelloWorldShouldFinishNormally) // NOLINT
    {
        auto process = createProcess();
        process->exec(PickYourPoisonPath(), { "--hello-world" });
        EXPECT_EQ(process->wait(milli(10), 500), ProcessStatus::FINISHED);
        std::string output = process->output();
        EXPECT_EQ(output, "Starting PickYourPoison Fault Injection Executable\nusing argument: --hello-world from command line\nHello, world!");
        EXPECT_EQ(process->exitCode(), 0);
    }



} // namespace


#endif