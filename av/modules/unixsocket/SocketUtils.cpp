/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"

#include <cassert>
#include <cstdint>

#include <unistd.h>
#include <cstdio>

void unixsocket::writeLength(int socketfd, int length)
{
    // TODO implement proper length writing
    if (length < 128)
    {
        uint8_t c = length;
        ::write(socketfd, &c, 1);
    }
    assert(false);
}

int unixsocket::readLength(int socketfd)
{
    int32_t total = 0;
    uint8_t byte;
    while (true)
    {
        int count = read(socketfd, &byte, 1);
        if (count == 1)
        {
            if ((byte & 0x80) == 0)
            {
                total = total * 128 + byte;
                return total;
            }
            total = total * 128 + (byte ^ 0x80);
            if (total > 128 * 1024)
            {
                return -1;
            }
        }
        else
        {
            perror("Reading length");
            return -1;
        }
    }
}
