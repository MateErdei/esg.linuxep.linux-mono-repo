/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


namespace unixsocket
{
    int readLength(int socketfd);
    void writeLength(int socketfd, int length);
}
