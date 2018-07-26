/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

class PipeForTests
{
public :
    PipeForTests();
    ~PipeForTests();

    void write( const std::string  & );

    int readFd();
private:
    int m_readFd;
    int m_writeFd;
};


