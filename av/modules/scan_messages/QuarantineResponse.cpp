/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "QuarantineResponse.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

QuarantineResponse::QuarantineResponse(QuarantineResult quarantineResult): m_result(quarantineResult)
{
}

QuarantineResponse::QuarantineResponse(Sophos::ssplav::QuarantineResponseMessage::Reader reader)
{
    m_result = QuarantineResult(reader.getResult());
}

std::string QuarantineResponse::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::QuarantineResponseMessage::Builder responseBuilder =
            message.initRoot<Sophos::ssplav::QuarantineResponseMessage>();

    responseBuilder.setResult(m_result);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}


QuarantineResult QuarantineResponse::getResult()
{
    return m_result;
}


