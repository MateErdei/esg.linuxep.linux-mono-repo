/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

class ReconnectScannerException : virtual public std::exception
{
public:
    explicit ReconnectScannerException(std::string errorMsg)
    : m_errorMsg(std::move(errorMsg))
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_errorMsg.c_str();
    }

private:
    std::string m_errorMsg;
};
