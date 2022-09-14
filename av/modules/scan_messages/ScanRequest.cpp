//Copyright 2020-2021, Sophos Limited.  All rights reserved.

#include "ScanRequest.h"

#include <unistd.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

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