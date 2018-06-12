//
// Created by pair on 12/06/18.
//

#include "gtest/gtest.h"
#include "Common/Process/IProcess.h"
#include "gmock/gmock.h"
#include <fstream>
using namespace Common::Process;
namespace
{
    TEST( ProcessImpl, SimpleEchoShouldReturnExpectedString)
    {
        auto process = createProcess();
        process->exec( "/bin/echo", {"hello"});
        EXPECT_EQ(process->wait(milli(10), 1), ProcessStatus::FINISHED);
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST( ProcessImpl, TouchFileCommandShouldCreateFile)
    {
        auto process = createProcess();
        ::remove( "success.txt");

        process->exec( "/usr/bin/touch", {"success.txt"});
        EXPECT_EQ(process->wait(milli(50), 1), ProcessStatus::FINISHED);
        EXPECT_EQ( process->exitCode(), 0);
        std::ifstream ifs2( "success.txt", std::ifstream::in);
        EXPECT_TRUE( ifs2);
        ::remove( "success.txt");
    }

    TEST( ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail)
    {
        auto process = createProcess();

        process->exec( "/usr/bin/touch", {""});
        EXPECT_EQ(process->wait(milli(50), 1), ProcessStatus::FINISHED);

        auto error = process->output();
        EXPECT_THAT( error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ( process->exitCode(), 1);
    }


    TEST( ProcessImpl, NonExistingCommandShouldFail)
    {
        auto process = createProcess();
        process->exec( "/bin/command_does_not_exists", {"fake_argument"});
        EXPECT_EQ(process->wait(milli(20), 5), ProcessStatus::FINISHED);
        EXPECT_EQ( process->exitCode(), 255);
    }



    TEST( ProcessImpl, LongCommandExecutionShouldTimeout)
    {
        std::fstream out( "test.sh", std::ofstream::out );
        out << "while true; do echo 'continue'; sleep 1; done\n";
        out.close();
        auto process = createProcess();
        process->exec( "/bin/bash", {"test.sh"});
        EXPECT_EQ(process->wait(milli(100), 1), ProcessStatus::TIMEOUT);

        process->kill();

        EXPECT_EQ(process->wait(milli(10), 1), ProcessStatus::FINISHED);

        auto output = process->output();

        EXPECT_THAT( output, ::testing::HasSubstr("continue"));
        ::remove( "test.sh");
    }


}
