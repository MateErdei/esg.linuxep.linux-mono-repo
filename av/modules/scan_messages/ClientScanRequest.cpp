//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ClientScanRequest.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <ScanRequest.capnp.h>

using namespace scan_messages;

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


std::string ClientScanRequest::getScanTypeAsStr() const
{
    switch (m_scanType)
    {
        case E_SCAN_TYPE_ON_ACCESS_OPEN:
        {
            return " (Open)";
        }
        break;
        case E_SCAN_TYPE_ON_ACCESS_CLOSE:
        {
            return " (Closed)";
        }
        break;
        default:
        {
            return " (On Demand)";
        }
    }
}