// Copyright 2023 Sophos All rights reserved.
#pragma once

#include <stdexcept>

namespace unixsocket
{
    struct EnvironmentInterruption : public std::exception
    {
        explicit EnvironmentInterruption(const char *where)
                : where_(where)
        {
        }

        const char *where_;

        [[nodiscard]] const char *what() const noexcept override
        {
            return "Environment interruption";
        }
    };
}
