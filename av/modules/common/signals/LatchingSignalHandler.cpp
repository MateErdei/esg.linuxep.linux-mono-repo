// Copyright 2022, Sophos Limited.  All rights reserved.

#include "LatchingSignalHandler.h"

using namespace common::signals;

bool LatchingSignalHandler::triggered()
{
    while (m_pipe.notified())
    {
        m_signalled = true;
    }
    return m_signalled;
}
