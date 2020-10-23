/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerSocket.h"

#include <utility>

FakeDetectionServer::FakeServerSocket::FakeServerSocket(const std::string& path, mode_t mode, uint8_t* Data, size_t Size) :
    FakeServerSocketBase(path, mode), m_Data(Data), m_Size(Size)

{
}
