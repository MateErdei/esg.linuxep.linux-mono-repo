// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <exception>
#include <stdexcept>

namespace ResponseActionsImpl
{
    /**
     * Exception class to report failures when handling response actions
     */
    class InvalidCommandFormat : public std::runtime_error
    {
    public:
        explicit InvalidCommandFormat(const std::string& what) : std::runtime_error("Invalid command format. " + what)
        {
        }
    };
} // namespace ResponseActionsImpl
