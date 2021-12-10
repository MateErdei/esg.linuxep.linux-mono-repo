/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ClientScanRequest.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <ScanRequest.capnp.h>

using namespace scan_messages;

void ClientScanRequest::setPath(const std::string& path)
{
    m_path = path;
}

void ClientScanRequest::setScanType(E_SCAN_TYPE scanType)
{
    m_scanType = scanType;
}

void ClientScanRequest::setUserID(const std::string& userID)
{
    m_userID = userID;
}

std::string ClientScanRequest::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname(m_path);
    requestBuilder.setScanInsideArchives(m_scanInsideArchives);
    requestBuilder.setScanInsideImages(m_scanInsideImages);
    requestBuilder.setScanType(m_scanType);
    requestBuilder.setUserID(m_userID);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}