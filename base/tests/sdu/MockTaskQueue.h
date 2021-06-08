/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <sdu/ITaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockTaskQueue : public RemoteDiagnoseImpl::ITaskQueue
{
public:
    MOCK_METHOD1(push, void(RemoteDiagnoseImpl::Task));
    MOCK_METHOD1(pushPriority, void(RemoteDiagnoseImpl::Task));
    MOCK_METHOD0(pop, RemoteDiagnoseImpl::Task());
    MOCK_METHOD1(pop, RemoteDiagnoseImpl::Task(bool));
};