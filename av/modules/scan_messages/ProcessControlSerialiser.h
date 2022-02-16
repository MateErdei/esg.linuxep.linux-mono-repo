/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.


******************************************************************************************************/

#pragma once

#include "ScanType.h"

#include <string>

namespace scan_messages
{
    enum E_COMMAND_TYPE: int
    {
        E_SHUTDOWN = 1,
        E_RELOAD = 2
    };

    class ProcessControlSerialiser
    {
    public:
        ProcessControlSerialiser() = delete;
        explicit ProcessControlSerialiser(E_COMMAND_TYPE requestType);

        void setCommandType(E_COMMAND_TYPE commandType);

        [[nodiscard]] std::string serialise() const;

    protected:
        E_COMMAND_TYPE m_commandType;
    };
}