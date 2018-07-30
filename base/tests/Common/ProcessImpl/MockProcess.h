/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Process/IProcess.h"
#include <gmock/gmock.h>
using namespace ::testing;


class MockProcess: public Common::Process::IProcess
{
public:
    MOCK_METHOD3( exec, void (const std::string & , const std::vector<std::string> & , const std::vector<Common::Process::EnvironmentPair>& ));
    MOCK_METHOD2( exec, void (const std::string & , const std::vector<std::string> & ));
    MOCK_METHOD2( wait, Common::Process::ProcessStatus(Common::Process::Milliseconds, int ));
    MOCK_METHOD0(kill, void(void));
    MOCK_METHOD0(exitCode, int(void));
    MOCK_METHOD0(output, std::string(void));
    MOCK_METHOD0(getStatus, Common::Process::ProcessStatus(void));
    MOCK_METHOD1(setOutputLimit, void(size_t));

};

