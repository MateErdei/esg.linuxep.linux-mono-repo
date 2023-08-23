// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <exception>
#include <string>

namespace ResponseActions::RACommon
{
    class toUtf8Exception : public std::exception
    {
    public:
        explicit toUtf8Exception(std::string message);

        const char* what() const noexcept override;

    private:
        std::string m_message;
    };
} // namespace ResponseActions::RACommon