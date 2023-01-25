// Copyright 2020-2023 Sophos Limited. All rights reserved.
/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseServerConnectionThread.h"

using namespace unixsocket;

BaseServerConnectionThread::BaseServerConnectionThread(const std::string& threadName) :
m_threadName(threadName)
{
}

void BaseServerConnectionThread::setIsRunning(bool value)
{
    m_isRunning = value;
}

bool BaseServerConnectionThread::isRunning() const
{
    return m_isRunning;
}
