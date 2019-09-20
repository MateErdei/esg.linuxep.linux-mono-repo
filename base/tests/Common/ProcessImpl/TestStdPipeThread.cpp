/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ProcessImpl/PipeHolder.h>
#include <Common/ProcessImpl/StdPipeThread.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

namespace
{
    TEST(TestStdPipeThread, TestConstruction) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        EXPECT_NO_THROW(Common::ProcessImpl::StdPipeThread t(pipe.readFd())); // NOLINT
    }

    TEST(TestStdPipeThread, TestOutputIsCollected) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output, expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsTrimmedIfSmallMaxLengthSpecified) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 5;
        t.setOutputLimit(limit);
        std::string captureOutput;
        t.setOutputTrimmedCallBack([&captureOutput](std::string out) { captureOutput += out; });
        t.start();
        std::string expected_output("After Sending Too many Characters 12345");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output, "12345");
        EXPECT_EQ(captureOutput, "After Sending Too many Characters ");
    }

    TEST(TestStdPipeThread, continousWritingToThePipeWillTrimInDifferentPlacesAsItDepends) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 5;
        t.setOutputLimit(limit);
        std::string captureOutput;
        t.setOutputTrimmedCallBack([&captureOutput](std::string out) { captureOutput += out; });
        t.start();
        std::string input_string("After Sending Too many Characters");
        for (char entry : input_string)
        {
            EXPECT_EQ(::write(pipe.writeFd(), &entry, 1), 1);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        // Now that the trim algorithm will be kicking at different times,
        // The output is not 'deterministic', but there are still expected results:
        // the concatenation of captured output + output must return the same string
        // output must always be between limit,2*limit
        std::string concat = captureOutput + actual_output;
        EXPECT_EQ(input_string, concat);
        EXPECT_GE(actual_output.size(), limit);
        EXPECT_LE(actual_output.size(), 2 * limit);
    }

    TEST(TestStdPipeThread, TestOutputCanBeExtractedOnTrimming) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 5;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        expected_output = expected_output.substr(expected_output.size() - limit);
        EXPECT_EQ(actual_output, expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsTrimmedIfMaxLengthSpecifiedCloseToTotalLength) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 14;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        // although the setLimit is set to 14, the system allow for up to 2x the threshold, hence
        // the whole string will be kept
        EXPECT_EQ(actual_output, expected_output);
    }

    TEST(TestStdPipeThread, TestOutputNotTrimmedIfSmallerThanOutputLimit) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 50;
        t.setOutputLimit(limit);
        t.start();
        std::string expected_output("EXPECTED OUTPUT");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Don't request stop - wait for EOF to terminate the thread instead.
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output, expected_output);
    }

    TEST(TestStdPipeThread, TestOutputIsReadFromPipeIfThreadIsStoppedBeforeItFinishesStarting) // NOLINT
    {
        Common::ProcessImpl::PipeHolder pipe;
        Common::ProcessImpl::StdPipeThread t(pipe.readFd());
        const size_t limit = 50;
        t.setOutputLimit(limit);
        std::string expected_output("EXPECTED OUTPUT");
        ssize_t ret = ::write(pipe.writeFd(), expected_output.c_str(), expected_output.size());
        ASSERT_EQ(ret, expected_output.size());
        pipe.closeWrite();
        // Note request for stop occurs before we request the thread to start
        t.requestStop();
        t.start();
        t.join();
        std::string actual_output = t.output();
        EXPECT_EQ(actual_output, expected_output);
    }

} // namespace