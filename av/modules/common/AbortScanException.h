/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>

class AbortScanException : virtual public std::exception
{
public:
    explicit AbortScanException(const std::string& errorMsg)
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
