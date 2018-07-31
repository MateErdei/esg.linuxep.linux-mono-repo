///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

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



