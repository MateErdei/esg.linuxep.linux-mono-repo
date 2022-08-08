// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "sync_versioned_files.h"

#include "Common/FileSystem/IFileSystem.h"

#include <gtest/gtest.h>

#include <fstream>

using namespace sync_versioned_files;

namespace
{
    class TestSyncVersionedFiles : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        void create_file(const fs::path& p)
        {
            std::ofstream(p).close();
        }

        void create_file(const fs::path& p, const std::string& contents)
        {
            std::ofstream s(p);
            s << contents;
            s.close();
        }

        std::string read_file(const fs::path& p)
        {
            return Common::FileSystem::fileSystem()->readFile(p);
        }

        fs::path m_testDir;
    };
} // namespace

TEST_F(TestSyncVersionedFiles, testStartsWith_true)
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    EXPECT_TRUE(startswith(a, s));
}


TEST_F(TestSyncVersionedFiles, testStartsWith_false)
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/f");
    EXPECT_FALSE(startswith(a, s));
}


TEST_F(TestSyncVersionedFiles, testSuffix)
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    fs::path actual_suffix = suffix(a, s);
    EXPECT_EQ(actual_suffix.string(), "d");
}

TEST_F(TestSyncVersionedFiles, testPathSlash)
{
    fs::path a("d");
    fs::path s2("/a/b/f");
    fs::path combined = s2 / a;
    EXPECT_EQ(combined.string(), "/a/b/f/d");
}

TEST_F(TestSyncVersionedFiles, testReplaceStem)
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    fs::path s2("/a/b/f");
    fs::path replace = replace_stem(a, s, s2);
    EXPECT_EQ(replace.string(), std::string("/a/b/f/d"));
}

TEST_F(TestSyncVersionedFiles, testDeleteRemovedFile_simpleFile)
{
    fs::path testfile = m_testDir / "testfile";
    create_file(testfile);

    EXPECT_TRUE(fs::exists(testfile));
    delete_removed_file(testfile);
    EXPECT_FALSE(fs::exists(testfile));
}

TEST_F(TestSyncVersionedFiles, testDeleteRemovedFile_singleExtension)
{
    fs::path testfile = m_testDir / "testfile.0";
    fs::path testfile2 = m_testDir / "testfile";
    create_file(testfile);
    create_file(testfile2);

    EXPECT_TRUE(fs::exists(testfile));
    EXPECT_TRUE(fs::exists(testfile2));
    delete_removed_file(testfile);
    EXPECT_FALSE(fs::exists(testfile));
    EXPECT_FALSE(fs::exists(testfile2));
}

TEST_F(TestSyncVersionedFiles, testDeleteRemovedFile_multipleExtensions)
{
    fs::path testfile = m_testDir / "testfile.1.0";
    fs::path testfile2 = m_testDir / "testfile.1";
    fs::path testfile3 = m_testDir / "testfile";
    create_file(testfile);
    create_file(testfile2);
    create_file(testfile3);

    EXPECT_TRUE(fs::exists(testfile));
    EXPECT_TRUE(fs::exists(testfile2));
    EXPECT_TRUE(fs::exists(testfile3));
    delete_removed_file(testfile);
    EXPECT_FALSE(fs::exists(testfile));
    EXPECT_FALSE(fs::exists(testfile2));
    EXPECT_FALSE(fs::exists(testfile3));
}

TEST_F(TestSyncVersionedFiles, testDeleteRemovedFile_missingFile)
{
    fs::path testfile = m_testDir / "testfile";

    EXPECT_NO_THROW(delete_removed_file(testfile));
}

TEST_F(TestSyncVersionedFiles, testDeleteRemovedFile_oneMissing)
{
    fs::path testfile = m_testDir / "testfile.0";
    fs::path testfile2 = m_testDir / "testfile";
    create_file(testfile);

    EXPECT_TRUE(fs::exists(testfile));
    delete_removed_file(testfile);
    EXPECT_FALSE(fs::exists(testfile));
}

TEST_F(TestSyncVersionedFiles, copyNoFiles)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
}

TEST_F(TestSyncVersionedFiles, copyDestExists)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
}

TEST_F(TestSyncVersionedFiles, copyNewFile)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);
    create_file(src / "exists");

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
    EXPECT_TRUE(fs::is_regular_file(dest / "exists"));
}

TEST_F(TestSyncVersionedFiles, copy2NewFiles)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);
    create_file(src / "exists");
    create_file(src / "exists2");

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
    EXPECT_TRUE(fs::is_regular_file(dest / "exists"));
    EXPECT_TRUE(fs::is_regular_file(dest / "exists2"));
}

TEST_F(TestSyncVersionedFiles, copyChangedFileSameSize)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);
    create_file(dest / "exists", "old_contents");
    std::string new_contents = "new_contents";
    create_file(src / "exists", new_contents);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
    EXPECT_TRUE(fs::is_regular_file(dest / "exists"));
    auto contents = read_file(dest / "exists");

    EXPECT_EQ(contents, new_contents);
}

TEST_F(TestSyncVersionedFiles, copyChangedFileDifferentSizes)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);
    create_file(dest / "exists", "old_contents that are a different size");
    std::string new_contents = "new_contents";
    create_file(src / "exists", new_contents);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(fs::is_directory(dest));
    EXPECT_TRUE(fs::is_regular_file(dest / "exists"));
    auto contents = read_file(dest / "exists");

    EXPECT_EQ(contents, new_contents);
}

TEST_F(TestSyncVersionedFiles, copyDeleteRemovedFile)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);
    create_file(dest / "exists", "old_contents");

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    EXPECT_FALSE(fs::is_regular_file(dest / "exists"));
}

TEST_F(TestSyncVersionedFiles, copyDoNothingForUnchangedFile)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);

    std::string old_contents = "old_contents";
    create_file(src / "exists", old_contents);
    create_file(dest / "exists", old_contents);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    ASSERT_TRUE(fs::is_regular_file(dest / "exists")); // Can't test contents if file doesn't exist
    auto contents = read_file(dest / "exists");
    EXPECT_EQ(contents, old_contents);
}

TEST_F(TestSyncVersionedFiles, copyDoNothingForUnchangedFileTestHardlink)
{
    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    auto dest_file = dest / "exists";
    fs::create_directory(src);
    fs::create_directory(dest);

    std::string old_contents = "old_contents";
    create_file(src / "exists", old_contents);
    create_file(dest_file, old_contents);
    fs::create_hard_link(dest_file, m_testDir / "exists");

    auto original_hard_link_count = fs::hard_link_count(dest_file);

    int ret = sync_versioned_files::copy(src, dest);
    EXPECT_EQ(ret, 0);
    ASSERT_TRUE(fs::is_regular_file(dest / "exists")); // Can't test contents if file doesn't exist
    auto contents = read_file(dest / "exists");
    EXPECT_EQ(contents, old_contents);

    auto new_hard_link_count = fs::hard_link_count(dest_file);
    EXPECT_EQ(new_hard_link_count, original_hard_link_count);
}

class SyncVersionedFilesParameterizedTest
    : public ::testing::TestWithParam<bool>
{
protected:
    void SetUp() override
    {
        const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_testDir = fs::temp_directory_path();
        m_testDir /= test_info->test_case_name();
        m_testDir /= test_info->name();
        fs::remove_all(m_testDir);
        fs::create_directories(m_testDir);
        fs::current_path(m_testDir);
    }

    void TearDown() override
    {
        fs::current_path(fs::temp_directory_path());
        fs::remove_all(m_testDir);
    }

    void create_file(const fs::path &p)
    {
        std::ofstream(p).close();
    }

    fs::path m_testDir;
};

INSTANTIATE_TEST_SUITE_P(TestSyncVersionedFiles, SyncVersionedFilesParameterizedTest, ::testing::Values(true, false));
TEST_P(SyncVersionedFilesParameterizedTest, testSyncVersionedFiles)
{
    bool isVersioned = GetParam();

    fs::path src = m_testDir / "src";
    fs::path dest = m_testDir / "dest";
    fs::create_directory(src);
    fs::create_directory(dest);

    create_file(src / "exists");
    create_file(dest / "exists.0");
    create_file(dest / "exists");

    create_file(dest / "doesnt_exist.0");
    create_file(dest / "doesnt_exist");

    fs::create_directory(dest / "directory");
    fs::create_symlink(".", dest / "symlink");

    int ret = sync_versioned_files::sync_versioned_files(src, dest, isVersioned);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(fs::exists(dest / "exists.0"), isVersioned);
    EXPECT_TRUE(fs::exists(dest / "exists"));

    EXPECT_FALSE(fs::exists(dest / "doesnt_exist.0"));
    EXPECT_FALSE(fs::exists(dest / "doesnt_exist"));

    EXPECT_TRUE(fs::exists(dest / "directory"));
    EXPECT_TRUE(fs::exists(dest / "symlink"));
}
