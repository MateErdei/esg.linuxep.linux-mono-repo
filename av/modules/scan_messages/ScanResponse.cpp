/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanResponse.h"

#include <ScanResponse.capnp.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

std::string scan_messages::ScanResponse::serialise()
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanResponse::Builder responseBuilder =
            message.initRoot<Sophos::ssplav::FileScanResponse>();

    responseBuilder.setClean(false);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}

void scan_messages::ScanResponse::setClean(bool b)
{
    m_clean = b;
}

scan_messages::ScanResponse::ScanResponse()
    : m_clean(false)
{
}
