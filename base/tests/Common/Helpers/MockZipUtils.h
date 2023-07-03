// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/ZipUtilities/IZipUtils.h"

#include <gmock/gmock.h>

using namespace ::testing;

class MockZipUtils : public Common::ZipUtilities::IZipUtils
{
public:
    MOCK_METHOD(
        int,
        zip,
        (const std::string& srcPath, const std::string& destPath, bool ignoreFileError),
        (const, override));
    MOCK_METHOD(
        int,
        zip,
        (const std::string& srcPath, const std::string& destPath, bool ignoreFileError, bool passwordProtected),
        (const, override));

    MOCK_METHOD(
        int,
        zip,
        (const std::string& srcPath,
         const std::string& destPath,
         bool ignoreFileError,
         bool passwordProtected,
         const std::string& password),
        (const, override));

    MOCK_METHOD(int, unzip, (const std::string& srcPath, const std::string& destPath), (const, override));
    MOCK_METHOD(
        int,
        unzip,
        (const std::string& srcPath, const std::string& destPath, bool passwordProtected, const std::string& password),
        (const, override));
};