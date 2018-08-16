/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Installer/ManifestDiff/Manifest.h>

#include "ExampleManifests.h"

#include <gtest/gtest.h>

namespace
{
    class TestManifest
            : public ::testing::Test
    {
    public:
    };

    Installer::ManifestDiff::Manifest manifestFromString(const std::string& s)
    {
        std::stringstream ost(s);
        return Installer::ManifestDiff::Manifest(ost);
    }
}

TEST_F(TestManifest, TestConstructionFromEmptyStream) //NOLINT
{
    std::stringstream ist("");

    EXPECT_NO_THROW(Installer::ManifestDiff::Manifest manifest(ist)); //NOLINT
}

TEST_F(TestManifest, TestConstructionFromStream) //NOLINT
{
    Installer::ManifestDiff::Manifest manifest(manifestFromString(sixteen_entries));
    EXPECT_EQ(manifest.size(),16);
}

TEST_F(TestManifest, TestWorkingOutAddition) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

    Installer::ManifestDiff::ManifestEntrySet added(new_manifest.calculateAdded(old_manifest));
    ASSERT_EQ(added.size(),1);

    EXPECT_EQ(added.begin()->path(),"files/base/bin/manifestdiff");
}


TEST_F(TestManifest, TestWorkingOutRemoval) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(two_entries));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry));

    Installer::ManifestDiff::ManifestEntrySet removed(new_manifest.calculateRemoved(old_manifest));
    ASSERT_EQ(removed.size(),1);
    EXPECT_EQ(removed.begin()->path(),"files/base/bin/manifestdiff");
}

TEST_F(TestManifest, TestWorkingOutChangesWhenNothingChanged) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry));

    Installer::ManifestDiff::ManifestEntrySet removed(new_manifest.calculateChanged(old_manifest));
    EXPECT_EQ(removed.size(),0);
}

TEST_F(TestManifest, TestWorkingOutChangesWhenOneFileChanged) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry_changed));

    Installer::ManifestDiff::ManifestEntrySet changed(new_manifest.calculateChanged(old_manifest));
    ASSERT_EQ(changed.size(),1);
    EXPECT_EQ(changed.begin()->path(),"files/base/bin/SulDownloader");
}

TEST_F(TestManifest, TestWorkingOutChangesWhenOneFileAdded) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

    Installer::ManifestDiff::ManifestEntrySet changed(new_manifest.calculateChanged(old_manifest));
    ASSERT_EQ(changed.size(),0);
}