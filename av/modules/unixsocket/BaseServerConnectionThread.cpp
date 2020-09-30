/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseServerConnectionThread.h"

void unixsocket::BaseServerConnectionThread::setIsRunning(bool value)
{
    m_isRunning = value;
}

bool unixsocket::BaseServerConnectionThread::isRunning() const
{
    return m_isRunning;
}
