/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineException.h"

using namespace UpdateSchedulerImpl::StateData;

StateMachineException::StateMachineException(std::string message) : m_message(std::move(message)) {}

const char* StateMachineException::what() const noexcept
{
    return m_message.c_str();
}
