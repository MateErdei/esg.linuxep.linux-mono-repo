/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <atomic>
#include<memory>

class IServiceExtension
{
public:
    virtual void Start(const std::string& socket,
            bool verbose,
            std::shared_ptr<std::atomic_bool> extensionFinished) = 0;
    virtual void Stop() = 0;
    virtual int GetExitCode() = 0;
};