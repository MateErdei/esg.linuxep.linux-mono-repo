// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ISignalHandlerBase.h"
#include "SignalHandlerBase.h"

namespace common::signals
{
    class LatchingSignalHandler : public SignalHandlerBase
    {
    public:
        using SignalHandlerBase::SignalHandlerBase;
        bool triggered() override;
    protected:
        bool m_signalled = false;
    };
}
