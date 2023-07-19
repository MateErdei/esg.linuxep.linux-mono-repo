// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once
#include <string>
#include <atomic>
#include <memory>

#define SVC_EXT_STOP_TIMEOUT 10

class IServiceExtension
{
public:
    virtual ~IServiceExtension() = default;
    virtual void Start(const std::string& socket,
            bool verbose,
            std::shared_ptr<std::atomic_bool> extensionFinished) = 0;
    virtual void Stop(long timeoutSeconds) = 0;
    virtual int GetExitCode() = 0;
};