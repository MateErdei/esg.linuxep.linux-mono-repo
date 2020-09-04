/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <avscanner/avscannerimpl/PathUtils.h>

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;
using namespace avscanner::avscannerimpl;

TEST(TestPathUtils, TestAppendForwardSlashToPath) // NOLINT
{
    fs::path pathWithNoSlash("/path/with/no/slash");
    fs::path pathWithSlash("/path/with/slash/");
    PathUtils::appendForwardSlashToPath(pathWithNoSlash);
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithSlash), pathWithSlash.string());
}

TEST(TestPathUtils, TestAppendForwardSlashToPathasd) // NOLINT
{
    fs::path pathWithNoSlash("\1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \a \b \t \n \v \f \r");
    fs::path pathWithSlash("/path/with/slash/");
    PathUtils::appendForwardSlashToPath(pathWithNoSlash);
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithNoSlash), pathWithNoSlash.string() + "/");
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithSlash), pathWithSlash.string());
}
