// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include <string>

namespace safestore
{
    class IQuarantineManager
    {
    public:
        virtual ~IQuarantineManager() = default;
        virtual bool quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) = 0;
    };
}