/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ProcessImpl/StdPipeThread.h>
#include <Common/ProcessImpl/PipeHolder.h>

#include <fstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    TEST(TestStdPipeThread, TestConstruction) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        EXPECT_NO_THROW(Common::ProcessImpl::StdPipeThread t(pipe.readFd())); //NOLINT
    }

    TEST(TestStdPipeThread, TestOutputIsCollected) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        int ret = ::write(pipe.writeFd(),expected_output.c_str(),expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output,expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsTrimmedIfSmallMaxLengthSpecified) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 5;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        int ret = ::write(pipe.writeFd(),expected_output.c_str(),expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        expected_output = expected_output.substr(expected_output.size() - limit);
        EXPECT_EQ(actual_output,expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsTrimmedIfMaxLengthSpecifiedCloseToTotalLength) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 14;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        int ret = ::write(pipe.writeFd(),expected_output.c_str(),expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        expected_output = expected_output.substr(expected_output.size() - limit);
        EXPECT_EQ(actual_output,expected_output);
    }

    TEST(TestStdPipeThread, TestOutputNotTrimmedIfSmallerThanOutputLimit) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 50;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        int ret = ::write(pipe.writeFd(),expected_output.c_str(),expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output,expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsReadFromPipeIfThreadIsStoppedBeforeItFinishesStarting) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 50;
        t.setOutputLimit(limit);
        std::string expected_output("EXPECTED OUTPUT");
        int ret = ::write(pipe.writeFd(),expected_output.c_str(),expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        //Note request for stop occurs before we request the thread to start
        t.requestStop();
        t.start();
        t.join();
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output,expected_output);
    }

}