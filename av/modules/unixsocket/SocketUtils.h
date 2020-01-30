/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace unixsocket
{
    int readLength(int socketfd);
    void writeLength(int socketfd, unsigned length);
    bool writeLengthAndBuffer(int socketfd, const std::string& buffer);
}
