///////////////////////////////////////////////////////////
//
// Copyright (C) 2004-2007 Sophos Plc, Oxford, England.
// All rights reserved.
//
//  NotifyPipe.cpp
//  Implementation of the Class NotifyPipe
//  Created on:      29-Apr-2008 14:10:31
//  Original author: Douglas Leeder
///////////////////////////////////////////////////////////

#include "NotifyPipe.h"

#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cassert>

using Common::Threads::NotifyPipe;

#define ASSERT assert
#define DBGOUT(x)

//LINKED-ATTRIBUTES
//LINKED-ATTRIBUTES-END

NotifyPipe::~NotifyPipe()
{
    ::close(m_readFd);
    ::close(m_writeFd);
}

NotifyPipe::NotifyPipe()
{
    int filedes[2];
    int ret = pipe(filedes);
    if (ret != 0)
    {
        // we need to exit here as there is no way we can recover if we
        // run out of file descriptors
        perror("NotifyPipe/pipe()");
        assert(ret == 0); // On debug builds we will get a useful error location
        exit(90);
    }

    m_readFd = filedes[0];
    ASSERT(m_readFd >= 0);
    m_writeFd = filedes[1];
    ASSERT(m_writeFd >= 0);

    if (fcntl(m_readFd,F_SETFL,O_NONBLOCK) == -1)
    {
        perror("NotifyPipe: fcntl(m_readFd, ...)" );
        exit(91);
    }

    if (fcntl(m_writeFd,F_SETFL,O_NONBLOCK) == -1)
    {
        perror("NotifyPipe: fcntl(m_writeFd, ...)" );
        exit(92);
    }


    DBGOUT("NotifyPipe: read="<<m_readFd<<", write="<<m_writeFd);
}

/**
 * @return True once for each time notify() is called.
 */
bool NotifyPipe::notified()
{
    char dummy;
    ssize_t amount = ::read(m_readFd,&dummy,1);
    DBGOUT("notified(" << m_readFd << ") = " << amount);
    return (amount == 1);
}

/**
 * Write a byte to the pipe.
 */
ssize_t NotifyPipe::notify()
{
    ssize_t ret = ::write(m_writeFd,"\0",1);
    DBGOUT("notify(" << m_writeFd << ") = " << ret << "errno="<<errno);
    return ret;
}

/**
 * Get the receiving side of the Pipe. For use in select calls etc.
 */
int NotifyPipe::readFd()
{
    return m_readFd;
}

/**
 * Get the sender side of the pipe. For use in signal handlers where the entire
 * object isn't wanted.
 */
int NotifyPipe::writeFd()
{
    return m_writeFd;
}

