// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IQuarantineManager.h"

namespace safestore
{
    class QuarantineManager : public IQuarantineManager
    {
    public:
        QuarantineManager();
        bool quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) override;
    };
}

