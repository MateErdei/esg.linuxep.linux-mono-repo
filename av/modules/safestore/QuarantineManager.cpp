// Copyright 2022, Sophos Limited.  All rights reserved.

#include "QuarantineManager.h"

#include "Logger.h"

using namespace safestore;

QuarantineManager::QuarantineManager() = default;

bool QuarantineManager::quarantineFile(
    const std::string& filePath,
    const std::string& threatId,
    const std::string& threatName,
    const std::string& sha256,
    datatypes::AutoFd autoFd)
{
    std::ignore = filePath;
    std::ignore = threatId;
    std::ignore = threatName;
    std::ignore = sha256;
    std::ignore = autoFd;

    // Currently doesn't quarantine
    return false;
}