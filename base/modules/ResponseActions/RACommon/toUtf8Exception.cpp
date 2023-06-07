// Copyright 2023 Sophos Limited. All rights reserved.

#include "toUtf8Exception.h"

using namespace ResponseActions::RACommon;

toUtf8Exception::toUtf8Exception(std::string message) : m_message(std::move(message)) {}

const char* toUtf8Exception::what() const noexcept
{
    return m_message.c_str();
}
