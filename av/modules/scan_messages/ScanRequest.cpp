// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "ScanRequest.h"

#include <capnp/message.h>

using namespace scan_messages;
using Builder = ScanRequest::Builder;
using Reader = ScanRequest::Reader;

ScanRequest::ScanRequest(Reader& requestMessage)
{
    setRequestFromMessage(requestMessage);
}

ScanRequest::~ScanRequest()
{
    close();
}

void ScanRequest::resetRequest(Reader& requestMessage)
{
    setRequestFromMessage(requestMessage);
}

void scan_messages::ScanRequest::setRequestFromMessage(Reader &requestMessage)
{
    setPath(requestMessage.getPathname());
    setScanInsideArchives(requestMessage.getScanInsideArchives());
    setScanInsideImages(requestMessage.getScanInsideImages());
    setScanType(static_cast<E_SCAN_TYPE>(requestMessage.getScanType()));
    setUserID(requestMessage.getUserID());
    setPid(requestMessage.getPid());
    setExecutablePath(requestMessage.getExecutablePath());
}

void scan_messages::ScanRequest::close()
{
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

bool ScanRequest::scanInsideImages() const
{
    return m_scanInsideImages;
}

std::string ScanRequest::getExecutablePath() const
{
    return m_executablePath;
}

int64_t ScanRequest::getPid() const
{
    return m_pid;
}

bool ScanRequest::operator==(const ScanRequest& rhs) const
{

    return m_path == rhs.m_path &&
           m_userID == rhs.m_userID &&
           m_scanType == rhs.m_scanType &&
           m_scanInsideArchives == rhs.m_scanInsideArchives &&
           m_scanInsideImages == rhs.m_scanInsideImages &&
           m_executablePath == rhs.m_executablePath &&
           m_pid == rhs.m_pid &&
           m_autoFd.get() == rhs.m_autoFd.get();
}