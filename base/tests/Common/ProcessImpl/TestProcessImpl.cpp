//
// Created by pair on 12/06/18.
//

#include "gtest/gtest.h"
#include "Common/Process/IProcess.h"
#include "gmock/gmock.h"
#include <fstream>
#include <Common/Process/IProcessException.h>

using namespace Common::Process;
namespace
{
    TEST( ProcessImpl, SimpleEchoShouldReturnExpectedString)
    {
        auto process = createProcess();
        process->exec( "/bin/echo", {"hello"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST( ProcessImpl, SupportMultiplesArgs)
    {
        auto process = createProcess();
        process->exec( "/bin/echo", {"hello","world"});
        ASSERT_EQ(process->output(), "hello world\n");
    }

    TEST( ProcessImpl, OutputBlockForResult)
    {
        for ( int i = 0; i< 10; i++)
        {
            auto process = createProcess();
            process->exec( "/bin/echo", {"hello"});
            ASSERT_EQ(process->output(), "hello\n");
        }
    }


    TEST( ProcessImpl, SupportAddingEnvironmentVariables)
    {
        std::string testFilename("envTestGood.sh");

        ::remove( testFilename.c_str());
        std::fstream out( testFilename, std::ofstream::out );
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        process->exec( "/bin/bash", {testFilename} );
        EXPECT_EQ(process->output(), "is\n");

        process = createProcess();
        process->exec( "/bin/bash", {testFilename}, {{"os","linux"},{"adj","great"}} );
        EXPECT_EQ(process->output(), "linux is great\n");
        ::remove( testFilename.c_str());

    }

    TEST( ProcessImpl, AddingInvalidEnvironmentVariableShouldFail)
    {
        std::string testFilename("envTestBad.sh");

        ::remove( testFilename.c_str());
        std::fstream out( testFilename, std::ofstream::out );
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        EXPECT_THROW(process->exec( "/bin/bash", {testFilename}, {{"","linux"},{"adj","great"}} ), Common::Process::IProcessException) ;

        ::remove( testFilename.c_str());

    }

    TEST( ProcessImpl, TouchFileCommandShouldCreateFile)
    {
        auto process = createProcess();
        ::remove( "success.txt");

        process->exec( "/usr/bin/touch", {"success.txt"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        EXPECT_EQ( process->exitCode(), 0);
        std::ifstream ifs2( "success.txt", std::ifstream::in);
        EXPECT_TRUE( ifs2);
        ::remove( "success.txt");
    }

    TEST( ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail)
    {
        auto process = createProcess();

        process->exec( "/usr/bin/touch", {""});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);

        auto error = process->output();
        EXPECT_THAT( error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ( process->exitCode(), 1);
    }

    TEST( ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail_WithoutCallingWait)
    {
        auto process = createProcess();
        process->exec( "/usr/bin/touch", {""});
        auto error = process->output();
        EXPECT_THAT( error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ( process->exitCode(), 1);
    }


    TEST( ProcessImpl, NonExistingCommandShouldFail)
    {
        auto process = createProcess();
        process->exec( "/bin/command_does_not_exists", {"fake_argument"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
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

        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);

        auto output = process->output();

        EXPECT_THAT( output, ::testing::HasSubstr("continue"));
        ::remove( "test.sh");
    }

    TEST( ProcessImpl, SupportOutputWithMultipleLines)
    {
        std::string targetfile = "/etc/passwd";

        std::ifstream t( targetfile, std::istream::in);
        std::string content((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        t.close();
        auto process = createProcess();
        process->exec( "/bin/cat", {targetfile});
        ASSERT_EQ(process->output(), content);
    }

    TEST( ProcessImpl, OutputCannotBeCalledBeforeExec)
    {
        auto process = createProcess();
        EXPECT_THROW(process->output(), IProcessException);
    }
    TEST( ProcessImpl, ExitCodeCannotBeCalledBeforeExec)
    {
        auto process = createProcess();
        EXPECT_THROW(process->exitCode(), IProcessException);
    }


}
