/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Installer/ManifestDiff/ManifestEntry.h>
#include <gtest/gtest.h>

TEST(TestManifestEntry, TestPosixPath) // NOLINT
{
    EXPECT_EQ(Installer::ManifestDiff::ManifestEntry::toPosixPath(".\\foo.dat"), "foo.dat");
}
