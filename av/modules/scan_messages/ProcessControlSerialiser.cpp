// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ProcessControlSerialiser.h"

#include "Logger.h"
#include "scan_messages/ProcessControl.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

ProcessControlSerialiser::ProcessControlSerialiser(E_COMMAND_TYPE requestType)
        : m_commandType(requestType)
{
}

std::string ProcessControlSerialiser::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::ProcessControl::Builder processControlBuilder =
            message.initRoot<Sophos::ssplav::ProcessControl>();

    processControlBuilder.setCommandType(m_commandType);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
}

