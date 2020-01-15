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
    m_path = requestMessage.getPathname();

}

int scan_messages::ScanRequest::fd()
{
    return m_fd.get();
}

std::string scan_messages::ScanRequest::path()
{
    return m_path;
}

void scan_messages::ScanRequest::close()
{
    m_fd.reset(-1);
    m_path = "";
}

void scan_messages::ScanRequest::setPath(const std::string& path)
{
    m_path = path;
}

void scan_messages::ScanRequest::setFd(int fd)
{
    m_fd.reset(fd);
}

Builder scan_messages::ScanRequest::serialise()
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname(m_path);

    return requestBuilder;
}
