/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessControlDeserialiser.h"

using namespace scan_messages;

ProcessControlDeserialiser::ProcessControlDeserialiser(Sophos::ssplav::ProcessControl::Reader& reader)
    : ProcessControlSerialiser(static_cast<E_COMMAND_TYPE>(reader.getCommandType()))
{
}

E_COMMAND_TYPE ProcessControlDeserialiser::getCommandType() const
{
    return m_commandType;
}
