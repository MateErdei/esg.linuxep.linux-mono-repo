///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////


#include "gtest/gtest.h"

#include <Installer/VersionedCopy/VersionedCopy.h>
#include <boost/filesystem.hpp>

#include <tests/Common/TestHelpers/TempDir.h>

namespace
{
    using Installer::VersionedCopy::VersionedCopy;

    void deleteTree(const Path& path)
    {
        boost::filesystem::remove_all(path);
    }

    void deleteTree(const Tests::TempDir& tempdir,
            const Path& rel_path)
    {
        deleteTree(tempdir.absPath(rel_path));
    }

    class TestVersionedCopy : public ::testing::Test
    {
    public:
        Common::FileSystem::IFileSystemPtr m_filesystem;
        void SetUp() override
        {
            m_filesystem = Common::FileSystem::createFileSystem();
        }

        void createFile(const Path& filename, const std::string& contents="FOOBAR")
        {
            m_filesystem->makedirs(m_filesystem->dirName(filename));
            m_filesystem->writeFile(filename,contents);
        }
    };

    class TestSame : public TestVersionedCopy
    {
    };

    int versionedCopyInTempDir(
            const Tests::TempDir& tempdir,
            const Path& filename,
            const Path& dist,
            const Path& inst
    )
    {
        return VersionedCopy::versionedCopy(
                tempdir.absPath(filename),
                tempdir.absPath(dist),
                tempdir.absPath(inst)
        );
    }

    int symlinkInTempDir(
            const Tests::TempDir& tempdir,
            const Path& target,
            const Path& dest
    )
    {
        return symlink(target.c_str(),
                       tempdir.absPath(dest).c_str()
        );
    }

    TEST_F(TestVersionedCopy, FirstInstall) //NOLINT
    {
        Tests::TempDir tempdir("","TestVersionedCopy_FirstInstall");
        const Path filename="distribution/files/a/b";
        const Path dist="distribution";
        const Path inst="installation";
        deleteTree(tempdir,inst);

        tempdir.createFile(filename,"FirstInstall");

        int ret = versionedCopyInTempDir(tempdir,filename, dist, inst);
        EXPECT_EQ(ret,0);

        const Path dest="installation/a/b";

        ASSERT_TRUE(tempdir.exists(dest));
        std::string contents = tempdir.fileContent(dest);
        EXPECT_EQ(contents,"FirstInstall");

        ASSERT_TRUE(tempdir.exists(dest+".0"));
        contents = tempdir.fileContent(dest+".0");
        EXPECT_EQ(contents,"FirstInstall");
    }

    TEST_F(TestVersionedCopy, LibraryLinks) //NOLINT
    {
        Tests::TempDir tempdir("","TestVersionedCopy_LibraryLinks");
        const Path rel_filename="a/libabc.so.1.4.5";
        const Path rel_dist_filename="distribution/"+rel_filename;
        const Path dist="distribution";
        const Path inst="installation";
        deleteTree(tempdir,inst);

        tempdir.createFile(rel_dist_filename,"FirstInstall");

        int ret = versionedCopyInTempDir(tempdir,rel_dist_filename, dist, inst);
        EXPECT_EQ(ret,0);

        const Path dest="installation/"+rel_filename;

        ASSERT_TRUE(tempdir.exists(dest));
        ASSERT_TRUE(tempdir.exists(dest+".0"));
        ASSERT_TRUE(tempdir.exists("installation/a/libabc.so.1.4"));
        ASSERT_TRUE(tempdir.exists("installation/a/libabc.so.1"));
        ASSERT_TRUE(tempdir.exists("installation/a/libabc.so"));
        ASSERT_FALSE(tempdir.exists("installation/a/libabc")); // Don't continue past .so
    }

    TEST_F(TestVersionedCopy, Upgrade) //NOLINT
    {
        Tests::TempDir tempdir("","TestVersionedCopy_Upgrade");

        const Path dist="distribution";
        const Path inst="installation";
        const Path filename=dist+"/files/a/b";
        const Path dest=inst+"/a/b";
        const Path dest0=dest+".0";
        const Path dest1=dest+".1";

        deleteTree(tempdir,inst);
        tempdir.createFile(filename,"Upgrade");
        tempdir.createFile(dest0,"FirstInstall");
        symlinkInTempDir(tempdir,"b.0",dest);

        int ret = versionedCopyInTempDir(
                tempdir,
                filename,
                dist,
                inst
                );
        EXPECT_EQ(ret,0);

        ASSERT_TRUE(tempdir.exists(dest));
        std::string contents = tempdir.fileContent(dest);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_TRUE(tempdir.exists(dest+".1"));
        contents = tempdir.fileContent(dest+".1");
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_FALSE(tempdir.exists(dest0));
    }

    TEST_F(TestVersionedCopy, SecondUpgrade) //NOLINT
    {
        Tests::TempDir tempdir("","TestVersionedCopy_SecondUpgrade");

        const Path dist="distribution";
        const Path inst="installation";
        const Path filename=dist+"/files/a/b";
        const Path dest=inst+"/a/b";
        const Path dest0=dest+".0";
        const Path dest1=dest+".1";
        const Path dest2=dest+".2";

        deleteTree(tempdir,inst);
        tempdir.createFile(filename,"Upgrade");
        tempdir.createFile(dest0,"FirstInstall");
        symlinkInTempDir(tempdir,"b.0",dest);

        int ret = versionedCopyInTempDir(tempdir,filename, dist, inst);
        EXPECT_EQ(ret,0);

        ASSERT_TRUE(tempdir.exists(dest));
        std::string contents = tempdir.fileContent(dest);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_TRUE(tempdir.exists(dest1));
        contents = tempdir.fileContent(dest1);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_FALSE(tempdir.exists(dest0));

        tempdir.createFile(filename,"SecondUpgrade");
        ret = versionedCopyInTempDir(tempdir, filename, dist, inst);
        EXPECT_EQ(ret,0);


        ASSERT_TRUE(tempdir.exists(dest));
        contents = tempdir.fileContent(dest);
        EXPECT_EQ(contents,"SecondUpgrade");
        ASSERT_FALSE(tempdir.exists(dest0));
        ASSERT_FALSE(tempdir.exists(dest1));

        ASSERT_TRUE(tempdir.exists(dest2));
        contents = tempdir.fileContent(dest2);
        EXPECT_EQ(contents,"SecondUpgrade");
    }


/**
 * Get a digit block from the end of s.
 * So Foo.14 returns 14
 * So Foo.1 returns 1
 * @param s
 * @return int
 */
    int getDigitFromEnd(const std::string& s)
    {
        return VersionedCopy::getDigitFromEnd(s);
    }


    TEST(TestGetDigitFromEnd, Empty) // NOLINT
    {
        int ret = getDigitFromEnd("");
        EXPECT_EQ(ret,-1);
    }

    TEST(TestGetDigitFromEnd, NoNumber) // NOLINT
    {
        int ret = getDigitFromEnd("ABC");
        EXPECT_EQ(ret,-1);
    }

    TEST(TestGetDigitFromEnd, Foo0) // NOLINT
    {
        int ret = getDigitFromEnd("Foo.0");
        EXPECT_EQ(ret,0);
    }

    TEST(TestGetDigitFromEnd, Foo1) // NOLINT
    {
        int ret = getDigitFromEnd("Foo.1");
        EXPECT_EQ(ret,1);
    }

    TEST(TestGetDigitFromEnd, Foo14) // NOLINT
    {
        int ret = getDigitFromEnd("Foo.14");
        EXPECT_EQ(ret,14);
    }

    TEST(TestGetDigitFromEnd, Just0) // NOLINT
    {
        int ret = getDigitFromEnd("0");
        EXPECT_EQ(ret,0);
    }

    TEST(TestGetDigitFromEnd, Just14) // NOLINT
    {
        int ret = getDigitFromEnd("14");
        EXPECT_EQ(ret,14);
    }

    bool sameInTempDir(
            const Tests::TempDir& tempdir,
            const Path& file1,
            const Path& file2
            )
    {
        return VersionedCopy::same(
                tempdir.absPath(file1),
                tempdir.absPath(file2)
                );
    }

    TEST_F(TestSame, BothDontExist) // NOLINT
    {
        Tests::TempDir tempdir("","TestSame_BothDontExist");
        deleteTree(tempdir,"installation/DoesntExist1");
        deleteTree(tempdir,"installation/DoesntExist2");
        bool ret = sameInTempDir(tempdir,"installation/DoesntExist1","installation/DoesntExist2");
        EXPECT_TRUE(ret);
    }

    TEST_F(TestSame, OneExists) // NOLINT
    {
        Tests::TempDir tempdir("","TestSame_OneExists");
        deleteTree(tempdir,"installation/1");
        tempdir.createFile("installation/2","FOO");

        bool ret = sameInTempDir(tempdir,"installation/1","installation/2");
        EXPECT_FALSE(ret);
    }

    TEST_F(TestSame, TwoDifferentLength)// NOLINT
    {
        Tests::TempDir tempdir("","TestSame_TwoDifferentLength");
        tempdir.createFile("installation/1","FOOBAR");
        tempdir.createFile("installation/2","FOO");

        bool ret = sameInTempDir(tempdir,"installation/1","installation/2");
        EXPECT_FALSE(ret);
    }

    TEST_F(TestSame, TwoDifferentContents)// NOLINT
    {
        Tests::TempDir tempdir("","TestSame_TwoDifferentContents");
        tempdir.createFile("installation/1","BAR");
        tempdir.createFile("installation/2","FOO");

        bool ret = sameInTempDir(tempdir,"installation/1","installation/2");
        EXPECT_FALSE(ret);
    }

    TEST_F(TestSame, TwoSame)// NOLINT
    {
        Tests::TempDir tempdir("","TestSame_TwoSame");
        tempdir.createFile("installation/1","FOO");
        tempdir.createFile("installation/2","FOO");

        bool ret = sameInTempDir(tempdir,"installation/1","installation/2");
        EXPECT_TRUE(ret);
    }
}
