// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ProcessControlSerialiser.h"

#include "scan_messages/ProcessControl.capnp.h"

namespace scan_messages
{
    class ProcessControlDeserialiser : public ProcessControlSerialiser
    {
    public:
        explicit ProcessControlDeserialiser(Sophos::ssplav::ProcessControl::Reader& reader);
        [[nodiscard]] E_COMMAND_TYPE getCommandType() const;
    };
}



