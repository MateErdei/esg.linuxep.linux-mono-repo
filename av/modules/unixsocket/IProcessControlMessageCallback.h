// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ProcessControlSerialiser.h"

#include <string>

class IProcessControlMessageCallback
{
public:
    virtual ~IProcessControlMessageCallback() = default;
    virtual void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) = 0;
};
