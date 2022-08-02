// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SignalHandlerBase.h"

using namespace common::signals;

int SignalHandlerBase::monitorFd()
{
    return m_pipe.readFd();
}
