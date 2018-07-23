///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef MANAGEMENTAGENT_STATUSRECEIVERIMPL_FAKEQUEUE_H
#define MANAGEMENTAGENT_STATUSRECEIVERIMPL_FAKEQUEUE_H

#include <Common/TaskQueueImpl/TaskQueueImpl.h>

namespace
{
    class FakeQueue : public Common::TaskQueueImpl::TaskQueueImpl
    {
    public:
        bool empty()
        {
            return m_tasks.empty();
        }
    };
}


#endif //MANAGEMENTAGENT_STATUSRECEIVERIMPL_FAKEQUEUE_H
