/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "filewalker/IFileWalkCallbacks.h"

#include "datatypes/sophos_filesystem.h"

#include <gmock/gmock.h>

namespace
{
    class MockCallbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        MOCK_METHOD2(processFile, void(const sophos_filesystem::path& filepath, bool symlinkTarget));
        MOCK_METHOD1(includeDirectory, bool(const sophos_filesystem::path& filepath));
        MOCK_METHOD2(userDefinedExclusionCheck, bool(const sophos_filesystem::path& filepath, bool isSymlink));
        MOCK_METHOD(void, registerError, (const std::ostringstream& errorString, std::error_code errorCode), (override));
    };
}