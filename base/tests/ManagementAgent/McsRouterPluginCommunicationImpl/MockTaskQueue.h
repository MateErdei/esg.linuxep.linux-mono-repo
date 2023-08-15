// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <gmock/gmock.h>
#include "Common/TaskQueue/ITaskQueue.h"

using namespace ::testing;
using ITaskPtr = Common::TaskQueue::ITaskPtr;

class MockTaskQueue : public virtual Common::TaskQueue::ITaskQueue
{
public:
    MOCK_METHOD1(queueTask, void(ITaskPtr task));
    MOCK_METHOD0(popTask, ITaskPtr());
};
