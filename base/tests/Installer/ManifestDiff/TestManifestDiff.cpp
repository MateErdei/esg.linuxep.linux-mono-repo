// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ExampleManifests.h"

#include "Installer/ManifestDiff/Manifest.h"
#include "Installer/ManifestDiff/ManifestDiff.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    Installer::ManifestDiff::Manifest manifestFromString(const std::string& s)
    {
        std::stringstream ost(s);
        return Installer::ManifestDiff::Manifest(ost);
    }

    class TestManifestDiff : public ::testing::Test
    {
    };

    // cppcheck-suppress syntaxError
    TEST_F(TestManifestDiff, calculateAddedWorksCorrectly)
    {
        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        Installer::ManifestDiff::PathSet added(
            Installer::ManifestDiff::ManifestDiff::calculateAdded(old_manifest, new_manifest));
        ASSERT_EQ(added.size(), 1);

        EXPECT_EQ(*(added.begin()), "files/base/bin/manifestdiff");
    }

    TEST_F(TestManifestDiff, writeAdded)
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        std::string dest = "test";

        EXPECT_CALL(*mockFileSystem, makedirs(_));
        EXPECT_CALL(*mockFileSystem, writeFile(dest, "files/base/bin/manifestdiff\n"));

        EXPECT_NO_THROW(Installer::ManifestDiff::ManifestDiff::writeAdded(dest, old_manifest, new_manifest));
    }

    TEST_F(TestManifestDiff, writeRemoved)
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(two_entries));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry));

        std::string dest = "test";

        EXPECT_CALL(*mockFileSystem, makedirs(_));
        EXPECT_CALL(*mockFileSystem, writeFile(dest, "files/base/bin/manifestdiff\n"));

        EXPECT_NO_THROW(
            Installer::ManifestDiff::ManifestDiff::writeRemoved(dest, old_manifest, new_manifest));
    }

    TEST_F(TestManifestDiff, writeChanged)
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry_changed));

        std::string dest = "test";

        EXPECT_CALL(*mockFileSystem, makedirs(_));
        EXPECT_CALL(*mockFileSystem, writeFile(dest, "files/base/bin/SulDownloader\n"));

        EXPECT_NO_THROW(
            Installer::ManifestDiff::ManifestDiff::writeChanged(dest, old_manifest, new_manifest));
    }
} // namespace