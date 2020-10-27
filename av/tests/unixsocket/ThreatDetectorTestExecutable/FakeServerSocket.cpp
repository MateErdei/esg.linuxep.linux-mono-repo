/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerSocket.h"

#include <utility>

FakeDetectionServer::FakeServerSocket::FakeServerSocket(const std::string& path, mode_t mode) :
    FakeServerSocketBase(path, mode)
{
}
void FakeDetectionServer::FakeServerSocket::start()
{
    AbstractThread::start();
    m_isRunning = true;
}

void FakeDetectionServer::FakeServerSocket::initializeData(uint8_t* Data, size_t Size)
{
    m_Data = Data;
    m_Size = Size;
}