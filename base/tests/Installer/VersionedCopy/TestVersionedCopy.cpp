///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////


#include <Installer/VersionedCopy/VersionedCopy.h>
#include "gtest/gtest.h"
#include <boost/filesystem.hpp>

namespace
{
    using Installer::VersionedCopy::VersionedCopy;

    class TestVersionedCopy : public ::testing::Test
    {
    public:
        Common::FileSystem::IFileSystemPtr m_filesystem;
        void SetUp() override
        {
            m_filesystem = Common::FileSystem::createFileSystem();
        }

        void deleteTree(const Path& path)
        {
            boost::filesystem::remove_all(path);
        }

        void createFile(const Path& filename, const std::string& contents="FOOBAR")
        {
            m_filesystem->makedirs(m_filesystem->dirName(filename));
            m_filesystem->writeFile(filename,contents);
        }
    };

    TEST_F(TestVersionedCopy, FirstInstall) //NOLINT
    {
        const Path filename="distribution/files/a/b";
        const Path dist="distribution";
        const Path inst="installation";
        deleteTree(inst);

        createFile(filename,"FirstInstall");

        int ret = VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        const Path dest="installation/a/b";

        ASSERT_TRUE(m_filesystem->exists(dest));
        std::string contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"FirstInstall");

        ASSERT_TRUE(m_filesystem->exists(dest+".0"));
        contents = m_filesystem->readFile(dest+".0");
        EXPECT_EQ(contents,"FirstInstall");
    }

    TEST_F(TestVersionedCopy, LibraryLinks) //NOLINT
    {
        const Path filename="distribution/files/a/libabc.so.1.4.5";
        const Path dist="distribution";
        const Path inst="installation";
        deleteTree(inst);

        createFile(filename,"FirstInstall");

        int ret = VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        const Path dest="installation/a/libabc.so.1.4.5";

        ASSERT_TRUE(m_filesystem->exists(dest));
        ASSERT_TRUE(m_filesystem->exists(dest+".0"));
        ASSERT_TRUE(m_filesystem->exists("installation/a/libabc.so.1.4"));
        ASSERT_TRUE(m_filesystem->exists("installation/a/libabc.so.1"));
        ASSERT_TRUE(m_filesystem->exists("installation/a/libabc.so"));
        ASSERT_FALSE(m_filesystem->exists("installation/a/libabc")); // Don't continue past .so
    }

    TEST_F(TestVersionedCopy, Upgrade) //NOLINT
    {

        const Path dist="distribution";
        const Path inst="installation";
        const Path filename=dist+"/files/a/b";
        const Path dest=inst+"/a/b";
        const Path dest0=dest+".0";
        const Path dest1=dest+".1";

        deleteTree(inst);
        createFile(filename,"Upgrade");
        createFile(dest0,"FirstInstall");
        symlink("b.0",dest.c_str());

        int ret = VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        ASSERT_TRUE(m_filesystem->exists(dest));
        std::string contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_TRUE(m_filesystem->exists(dest+".1"));
        contents = m_filesystem->readFile(dest+".1");
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_FALSE(m_filesystem->exists(dest0));
    }

    TEST_F(TestVersionedCopy, SecondUpgrade) //NOLINT
    {

        const Path dist="distribution";
        const Path inst="installation";
        const Path filename=dist+"/files/a/b";
        const Path dest=inst+"/a/b";
        const Path dest0=dest+".0";
        const Path dest1=dest+".1";
        const Path dest2=dest+".2";

        deleteTree(inst);
        createFile(filename,"Upgrade");
        createFile(dest0,"FirstInstall");
        symlink("b.0",dest.c_str());

        int ret = VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        ASSERT_TRUE(m_filesystem->exists(dest));
        std::string contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_TRUE(m_filesystem->exists(dest1));
        contents = m_filesystem->readFile(dest1);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_FALSE(m_filesystem->exists(dest0));

        createFile(filename,"SecondUpgrade");
        ret = VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);


        ASSERT_TRUE(m_filesystem->exists(dest));
        contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"SecondUpgrade");
        ASSERT_FALSE(m_filesystem->exists(dest0));
        ASSERT_FALSE(m_filesystem->exists(dest1));

        ASSERT_TRUE(m_filesystem->exists(dest2));
        contents = m_filesystem->readFile(dest2);
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
}
