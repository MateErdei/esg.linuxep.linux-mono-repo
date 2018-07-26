//
// Created by pair on 25/06/18.
//

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


