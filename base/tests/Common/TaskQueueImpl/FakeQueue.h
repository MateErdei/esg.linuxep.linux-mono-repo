// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/TaskQueueImpl/TaskQueueImpl.h"

namespace
{
    class FakeQueue : public Common::TaskQueueImpl::TaskQueueImpl
    {
    public:
        bool empty() { return m_tasks.empty(); }
    };
} // namespace
