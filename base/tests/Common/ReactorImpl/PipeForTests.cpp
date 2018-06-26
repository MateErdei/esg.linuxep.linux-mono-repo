//
// Created by pair on 25/06/18.
//

#include <string>
#include <cassert>
#include <unistd.h>
#include "PipeForTests.h"

PipeForTests::PipeForTests()
{
    int filedes[2];
    int ret = ::pipe(filedes);
    assert( ret == 0);
    m_readFd = filedes[0];
    assert(m_readFd >= 0);
    m_writeFd = filedes[1];
    assert(m_writeFd >= 0);
}

PipeForTests::~PipeForTests()
{
    ::close(m_readFd);
    ::close(m_writeFd);
}

void PipeForTests::write(const std::string & data)
{
    ssize_t nbytes = ::write( m_writeFd, data.data(), data.size());
    assert(nbytes == data.size());
}

int PipeForTests::readFd()
{
    return m_readFd;
}
