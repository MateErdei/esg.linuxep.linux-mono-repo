// Copyright 2023 Sophos Limited. All rights reserved.

#include "sessionrunner/SessionRunnerImpl.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>

using namespace ::testing;

sessionrunner::SessionRunnerStatus statusFromExitResult( int exitCode)
{
    sessionrunner::SessionRunnerStatus st;
    sessionrunner::SessionRunnerImpl::setStatusFromExitResult("1234", st, exitCode, "");
    return st; 
}

class SessionRunnerImpl : public LogOffInitializedTests{}; 

TEST_F(SessionRunnerImpl, setStatusFromExitResult_shouldNotTryToProcessFurtherOnExitCodeDifferentFrom0) // NOLINT
{
    for(int i=1;i<255;i++)
    {
        auto status = statusFromExitResult(i);
        EXPECT_EQ(status.errorCode, sessionrunner::ErrorCode::UNEXPECTEDERROR);
    }
}

TEST_F(SessionRunnerImpl, setStatusFromExitResult_zeroShouldReturnSuccess) // NOLINT
{
    auto status = statusFromExitResult( 0);
    EXPECT_EQ(status.errorCode,  sessionrunner::ErrorCode::SUCCESS);
}

