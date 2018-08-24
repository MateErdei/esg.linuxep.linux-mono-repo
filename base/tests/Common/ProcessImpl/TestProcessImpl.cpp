/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "Common/Process/IProcess.h"
#include "gmock/gmock.h"
#include <fstream>
#include <Common/Process/IProcessException.h>

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
        ASSERT_EQ(ret,0);
    }

    TEST( ProcessImpl, SimpleEchoShouldReturnExpectedString) //NOLINT
    {
        auto process = createProcess();
        process->exec( "/bin/echo", {"hello"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        ASSERT_EQ(process->output(), "hello\n");
    }

    TEST( ProcessImpl, SupportMultiplesArgs) //NOLINT
    {
        auto process = createProcess();
        process->exec( "/bin/echo", {"hello","world"});
        ASSERT_EQ(process->output(), "hello world\n");
    }

    TEST( ProcessImpl, OutputBlockForResult) //NOLINT
    {
        for ( int i = 0; i< 10; i++)
        {
            auto process = createProcess();
            process->exec( "/bin/echo", {"hello"});
            ASSERT_EQ(process->output(), "hello\n");
        }
    }


    TEST( ProcessImpl, SupportAddingEnvironmentVariables) //NOLINT
    {
        std::string testFilename("envTestGood.sh");

        removeFile(testFilename);
        std::fstream out( testFilename, std::ofstream::out );
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        process->exec( "/bin/bash", {testFilename} );
        EXPECT_EQ(process->output(), "is\n");

        process = createProcess();
        process->exec( "/bin/bash", {testFilename}, {{"os","linux"},{"adj","great"}} );
        EXPECT_EQ(process->output(), "linux is great\n");
        removeFile(testFilename);

    }

    TEST( ProcessImpl, AddingInvalidEnvironmentVariableShouldFail) //NOLINT
    {
        std::string testFilename("envTestBad.sh");

        removeFile(testFilename);
        std::fstream out( testFilename, std::ofstream::out );
        out << "echo ${os} is ${adj}\n";
        out.close();
        auto process = createProcess();
        EXPECT_THROW(process->exec( "/bin/bash", {testFilename}, {{"","linux"},{"adj","great"}} ), Common::Process::IProcessException) ;  //NOLINT

        removeFile(testFilename);

    }

    TEST( ProcessImpl, TouchFileCommandShouldCreateFile) //NOLINT
    {
        auto process = createProcess();
        removeFile("success.txt");

        process->exec( "/usr/bin/touch", {"success.txt"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        EXPECT_EQ( process->exitCode(), 0);
        std::ifstream ifs2( "success.txt", std::ifstream::in);
        EXPECT_TRUE( ifs2);
        removeFile("success.txt");
    }

    TEST( ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail) //NOLINT
    {
        auto process = createProcess();

        process->exec( "/usr/bin/touch", {""});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);

        auto error = process->output();
        EXPECT_THAT( error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ( process->exitCode(), 1);
    }

    TEST( ProcessImpl, CommandNotPassingExpectingArgumentsShouldFail_WithoutCallingWait) //NOLINT
    {
        auto process = createProcess();
        process->exec( "/usr/bin/touch", {""});
        auto error = process->output();
        EXPECT_THAT( error, ::testing::HasSubstr("No such file or directory"));
        EXPECT_EQ( process->exitCode(), 1);
    }


    TEST( ProcessImpl, NonExistingCommandShouldFail) //NOLINT
    {
        auto process = createProcess();
        process->exec( "/bin/command_does_not_exists", {"fake_argument"});
        EXPECT_EQ(process->wait(milli(1), 500), ProcessStatus::FINISHED);
        EXPECT_EQ( process->output(), "");
        EXPECT_EQ(process->exitCode(), 2);
    }



    TEST( ProcessImpl, LongCommandExecutionShouldTimeout) //NOLINT
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
        removeFile("test.sh");
    }

    TEST( ProcessImpl, SupportOutputWithMultipleLines) //NOLINT
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

    TEST( ProcessImpl, OutputCannotBeCalledBeforeExec) //NOLINT
    {
        auto process = createProcess();
        EXPECT_THROW(process->output(), IProcessException); //NOLINT
    }
    TEST( ProcessImpl, ExitCodeCannotBeCalledBeforeExec) //NOLINT
    {
        auto process = createProcess();
        EXPECT_THROW(process->exitCode(), IProcessException); //NOLINT
    }


}
