/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "filewalker/IFileWalkCallbacks.h"

#include "datatypes/sophos_filesystem.h"

#include <gmock/gmock.h>

/*
 * hack to allow gtest/gmock to interact with mock methods passing fs::path parameters
 *
 * See: https://github.com/google/googletest/issues/521
 *      https://github.com/google/googletest/issues/1614
 *      https://github.com/google/googletest/pull/1186
 *
 * hopefully fixed in googletest v1.10.0
 */
#if __cplusplus >= 201703L && __has_include(<filesystem>)
namespace std::filesystem // NOLINT
#else
    namespace std::experimental::filesystem // NOLINT
#endif
{
    inline void PrintTo(const path& p, std::ostream* os)
    {
        *os << p;
    }
}

namespace
{
    class MockCallbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        MOCK_METHOD2(processFile, void(const sophos_filesystem::path& filepath, bool symlinkTarget));
        MOCK_METHOD1(includeDirectory, bool(const sophos_filesystem::path& filepath));
        MOCK_METHOD2(userDefinedExclusionCheck, bool(const sophos_filesystem::path& filepath, bool isSymlink));
        MOCK_METHOD1(registerError, void(const std::ostringstream& errorString));
    };
}