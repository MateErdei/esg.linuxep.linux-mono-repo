// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <exception>
#include <stdexcept>

namespace ResponseActionsImpl
    {
        /**
         * Exception class to report failures when handling response actions
         */
        class ResponseActionsException : public std::runtime_error
        {
        public:
            explicit ResponseActionsException(const std::string& what) : std::runtime_error(what) {}
        };
    } // namespace PluginApi


