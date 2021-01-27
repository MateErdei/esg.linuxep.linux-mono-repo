/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineException.h"

using namespace UpdateScheduler::StateData;

StateMachineException::StateMachineException(std::string message) : m_message(std::move(message)) {}

const char* StateMachineException::what() const noexcept
{
    return m_message.c_str();
}
