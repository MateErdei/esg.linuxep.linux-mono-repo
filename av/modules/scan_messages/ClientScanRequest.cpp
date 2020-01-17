/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ClientScanRequest.h"

#include "ScanRequest.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

void scan_messages::ClientScanRequest::setPath(const std::string& path)
{
    m_path = path;
}

std::string scan_messages::ClientScanRequest::serialise()
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname(m_path);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}

std::string scan_messages::ClientScanRequest::path()
{
    return m_path;
}
