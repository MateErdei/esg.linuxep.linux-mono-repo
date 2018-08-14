/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/Installer/ManifestDiff/ManifestEntry.h>
#include "gtest/gtest.h"

TEST(TestManifestEntry, TestPosixPath) //NOLINT
{
    EXPECT_EQ(
            Installer::ManifestDiff::ManifestEntry::toPosixPath(".\\foo.dat"),
            "foo.dat"
            );
}
