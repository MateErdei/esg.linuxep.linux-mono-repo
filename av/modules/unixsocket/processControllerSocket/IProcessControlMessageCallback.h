// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ProcessControlSerialiser.h"

#include <string>

namespace unixsocket
{
    class IProcessControlMessageCallback
    {
    public:
        virtual ~IProcessControlMessageCallback() = default;
        virtual void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) = 0;
    };

    using IProcessControlMessageCallbackPtr = std::shared_ptr<IProcessControlMessageCallback>;
}