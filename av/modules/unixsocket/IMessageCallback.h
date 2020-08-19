/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

class IMessageCallback
{
public:
    virtual ~IMessageCallback() = default;
    virtual void processMessage(const std::string& message) = 0;
};
