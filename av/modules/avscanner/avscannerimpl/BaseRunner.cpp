/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseRunner.h"

using namespace avscanner::avscannerimpl;

void BaseRunner::setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr)
{
    m_socket = std::move(ptr);
}
