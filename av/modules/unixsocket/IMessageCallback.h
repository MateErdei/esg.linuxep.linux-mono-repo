/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

class IMessageCallback
{
public:
    virtual ~IMessageCallback() = default;
    virtual void processMessage(const std::string& message) = 0;
};
