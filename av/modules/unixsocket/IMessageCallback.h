/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

class IMessageCallback
{
public:
    virtual void processMessage(const std::string& message) = 0;
};
