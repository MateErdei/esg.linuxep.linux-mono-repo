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

void FakeDetectionServer::FakeServerSocket::initializeData(std::shared_ptr<std::vector<uint8_t>> data)
{
    m_data = std::move(data);
}