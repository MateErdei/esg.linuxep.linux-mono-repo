/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PipeForTests.h"

#include <cassert>
#include <string>
#include <unistd.h>

PipeForTests::PipeForTests()
{
    int filedes[2];
    int ret = ::pipe(filedes);
    assert(ret == 0);
    m_readFd = filedes[0];
    assert(m_readFd >= 0);
    m_writeFd = filedes[1];
    assert(m_writeFd >= 0);
    static_cast<void>(ret);
}

PipeForTests::~PipeForTests()
{
    ::close(m_readFd);
    ::close(m_writeFd);
}

void PipeForTests::write(const std::string& data)
{
    ssize_t nbytes = ::write(m_writeFd, data.data(), data.size());
    assert(nbytes == (ssize_t)data.size());
    static_cast<void>(nbytes);
}

int PipeForTests::readFd()
{
    return m_readFd;
}
