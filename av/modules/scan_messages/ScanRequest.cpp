//
// Created by pair on 13/01/2020.
//

#include "ScanRequest.h"

#include <unistd.h>

scan_messages::ScanRequest::ScanRequest()
        : m_fd(-1)
{
}

scan_messages::ScanRequest::ScanRequest(int fd, Sophos::ssplav::FileScanRequest::Reader &requestMessage)
    : m_fd(fd)
{
    setRequestFromMessage(requestMessage);
}

scan_messages::ScanRequest::~ScanRequest()
{
    close();
}

void scan_messages::ScanRequest::resetRequest(int fd, Sophos::ssplav::FileScanRequest::Reader& requestMessage)
{
    close();
    m_fd = fd;
    setRequestFromMessage(requestMessage);
}

void scan_messages::ScanRequest::setRequestFromMessage(Sophos::ssplav::FileScanRequest::Reader &requestMessage)
{
    m_path = requestMessage.getPathname();

}

int scan_messages::ScanRequest::fd()
{
    return m_fd;
}

std::string scan_messages::ScanRequest::path()
{
    return m_path;
}

void scan_messages::ScanRequest::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
    m_path = "";
}
