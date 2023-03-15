// Copyright 2018-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/Process/IProcess.h"

#include <gmock/gmock.h>
#include <string>
#include <functional>

using namespace ::testing;

class MockProcess : public Common::Process::IProcess
{
public:
    MOCK_METHOD5(
        exec,
        void(
            const std::string&,
            const std::vector<std::string>&,
            const std::vector<Common::Process::EnvironmentPair>&,
            uid_t,
            gid_t));
    MOCK_METHOD3(
        exec,
        void(
            const std::string&,
            const std::vector<std::string>&,
            const std::vector<Common::Process::EnvironmentPair>&));
    MOCK_CONST_METHOD0(childPid, int()); 
    MOCK_METHOD2(exec, void(const std::string&, const std::vector<std::string>&));
    MOCK_METHOD2(wait, Common::Process::ProcessStatus(Common::Process::Milliseconds, int));
    MOCK_METHOD0(kill, bool(void));
    MOCK_METHOD1(kill, bool(int));
    MOCK_METHOD0(exitCode, int(void));
    MOCK_METHOD(int, nativeExitCode, (), (override));
    MOCK_METHOD0(output, std::string(void));
    MOCK_METHOD0(getStatus, Common::Process::ProcessStatus(void));
    MOCK_METHOD1(setOutputLimit, void(size_t));
    MOCK_METHOD1(setFlushBufferOnNewLine, void(bool));
    MOCK_METHOD0(waitUntilProcessEnds, void(void));
    MOCK_METHOD1(setCoreDumpMode, void(const bool));
    MOCK_METHOD1(setOutputTrimmedCallback, void(std::function<void(std::string)>));
    MOCK_METHOD(std::string, errorOutput, (), (override));
    MOCK_METHOD(std::string, standardOutput, (), (override));
    // gmock issued a compiler error with the mock below. Hence, the full definition.
    void setNotifyProcessFinishedCallBack(Common::Process::IProcess::functor) override {}
};
