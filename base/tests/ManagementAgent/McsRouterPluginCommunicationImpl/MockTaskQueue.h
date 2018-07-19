/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKTASKQUEUE_H
#define MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKTASKQUEUE_H

#include <modules/Common/TaskQueue/ITaskQueue.h>

#include <gmock/gmock.h>

using namespace ::testing;
using ITaskPtr = Common::TaskQueue::ITaskPtr;

class MockTaskQueue : public virtual Common::TaskQueue::ITaskQueue
{
public:
    MOCK_METHOD1(queueTask, void(ITaskPtr& task));
    MOCK_METHOD0(popTask, ITaskPtr());
};

#endif //MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKTASKQUEUE_H
