/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Installer/ManifestDiff/Manifest.h>
#include <Installer/ManifestDiff/ManifestDiff.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>

#include "ExampleManifests.h"

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    Installer::ManifestDiff::Manifest manifestFromString(const std::string& s)
    {
        std::stringstream ost(s);
        return Installer::ManifestDiff::Manifest(ost);
    }


    class TestManifestDiff
            : public ::testing::Test
    {
    public:
        void TearDown() override
        {
            Common::FileSystem::restoreFileSystem();
        }
    };

    TEST_F(TestManifestDiff, calculateAddedWorksCorrectly) //NOLINT
    {
        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        Installer::ManifestDiff::PathSet added(
                Installer::ManifestDiff::ManifestDiff::calculateAdded(old_manifest, new_manifest));
        ASSERT_EQ(added.size(), 1);

        EXPECT_EQ(*(added.begin()), "files/base/bin/manifestdiff");
    }

    TEST_F(TestManifestDiff, writeAdded) //NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        std::string dest = "test";

        EXPECT_CALL(*mockFileSystem, writeFile(dest,"files/base/bin/manifestdiff\n"));

        EXPECT_NO_THROW(Installer::ManifestDiff::ManifestDiff::writeAdded(dest, old_manifest, new_manifest)); // NOLINT
    }
}