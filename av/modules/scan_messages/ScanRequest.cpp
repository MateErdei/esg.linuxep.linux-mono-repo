/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanRequest.h"

#include <unistd.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;
using Builder = ScanRequest::Builder;
using Reader = ScanRequest::Reader;

ScanRequest::ScanRequest(int fd, Reader& requestMessage)
    : m_fd(fd)
{
    setRequestFromMessage(requestMessage);
}

ScanRequest::~ScanRequest()
{
    close();
}

void ScanRequest::resetRequest(int fd, Reader& requestMessage)
{
    m_fd.reset(fd);
    setRequestFromMessage(requestMessage);
}

void scan_messages::ScanRequest::setRequestFromMessage(Reader &requestMessage)
{
    setPath(requestMessage.getPathname());
    setScanInsideArchives(requestMessage.getScanInsideArchives());
}

int scan_messages::ScanRequest::fd()
{
    return m_fd.get();
}

void scan_messages::ScanRequest::close()
{
    m_fd.reset(-1);
    setPath("");
}

std::string ScanRequest::path() const
{
    return m_path;
}

bool ScanRequest::scanInsideArchives() const
{
    return m_scanInsideArchives;
}
