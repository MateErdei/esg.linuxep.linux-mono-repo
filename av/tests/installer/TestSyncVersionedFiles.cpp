/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sync_versioned_files.h"

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

        void create_file(const fs::path &p)
        {
            std::ofstream(p).close();
        }

        fs::path m_testDir;
    };
} // namespace

TEST_F(TestSyncVersionedFiles, testStartsWith_true) // NOLINT
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    EXPECT_TRUE(startswith(a, s));
}


TEST_F(TestSyncVersionedFiles, testStartsWith_false) // NOLINT
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/f");
    EXPECT_FALSE(startswith(a, s));
}


TEST_F(TestSyncVersionedFiles, testSuffix) // NOLINT
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    fs::path actual_suffix = suffix(a, s);
    EXPECT_EQ(actual_suffix.string(), "d");
}

TEST_F(TestSyncVersionedFiles, testPathSlash) // NOLINT
{
    fs::path a("d");
    fs::path s2("/a/b/f");
    fs::path combined = s2 / a;
    EXPECT_EQ(combined.string(), "/a/b/f/d");
}

TEST_F(TestSyncVersionedFiles, testReplaceStem) // NOLINT
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

INSTANTIATE_TEST_CASE_P(TestSyncVersionedFiles, SyncVersionedFilesParameterizedTest, ::testing::Values(true, false)); // NOLINT

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
