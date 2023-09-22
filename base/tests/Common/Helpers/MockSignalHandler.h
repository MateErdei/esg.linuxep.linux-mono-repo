// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ProcessMonitoring/ISignalHandler.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class MockSignalHandler : public Common::ISignalHandler
    {
    public:
        MockSignalHandler()
        {
            ON_CALL(*this, subprocessExitFileDescriptor).WillByDefault(Return(1));
            ON_CALL(*this, terminationFileDescriptor).WillByDefault(Return(2));
            ON_CALL(*this, usr1FileDescriptor).WillByDefault(Return(3));
        }
        MOCK_METHOD(bool, clearUSR1Pipe,(), (override));
        MOCK_METHOD(bool, clearSubProcessExitPipe, (),(override));
        MOCK_METHOD(bool, clearTerminationPipe, (),(override));
        MOCK_METHOD(int, subprocessExitFileDescriptor, (),(override));
        MOCK_METHOD(int, terminationFileDescriptor, (),(override));
        MOCK_METHOD(int, usr1FileDescriptor, (),(override));
    };
}