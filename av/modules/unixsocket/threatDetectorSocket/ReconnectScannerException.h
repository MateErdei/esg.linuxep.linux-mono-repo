/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>

class ReconnectScannerException : virtual public std::exception
{
public:
    explicit ReconnectScannerException(const std::string& errorMsg)
    : m_errorMsg(errorMsg)
    {

    }

    virtual const char* what() const throw ()
    {
        return m_errorMsg.c_str();
    }

private:
    std::string m_errorMsg;
};
