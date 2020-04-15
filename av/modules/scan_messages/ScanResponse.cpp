/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanResponse.h"

#include <ScanResponse.capnp.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

ScanResponse::ScanResponse()
        : m_clean(false)
{
}

std::string ScanResponse::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanResponse::Builder responseBuilder =
            message.initRoot<Sophos::ssplav::FileScanResponse>();

    responseBuilder.setClean(m_clean);
    responseBuilder.setThreatName(m_threatName);
    responseBuilder.setFullScanResult(m_fullScanResult);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}

void ScanResponse::setClean(bool b)
{
    m_clean = b;
}

void ScanResponse::setThreatName(std::string threatName)
{
    m_threatName = std::move(threatName);
}

void ScanResponse::setFullScanResult(std::string threatName)
{
    m_fullScanResult = std::move(threatName);
}

