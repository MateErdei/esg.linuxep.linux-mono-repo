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

        int ret = Installer::VersionedCopy::VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        const Path dest="installation/a/b";

        ASSERT_TRUE(m_filesystem->exists(dest));
        std::string contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"FirstInstall");

        ASSERT_TRUE(m_filesystem->exists(dest+".0"));
        contents = m_filesystem->readFile(dest+".0");
        EXPECT_EQ(contents,"FirstInstall");
    }

    TEST_F(TestVersionedCopy, Upgrade) //NOLINT
    {

        const Path dist="distribution";
        const Path inst="installation";
        const Path filename=dist+"/files/a/b";
        const Path dest=inst+"/a/b";
        const Path dest0=dest+".0";
        const Path dest1=dest+".1";

        createFile(filename,"Upgrade");
        createFile(dest0,"FirstInstall");
        symlink("b.0",dest.c_str());

        int ret = Installer::VersionedCopy::VersionedCopy::versionedCopy(filename, dist, inst);
        EXPECT_EQ(ret,0);

        ASSERT_TRUE(m_filesystem->exists(dest));
        std::string contents = m_filesystem->readFile(dest);
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_TRUE(m_filesystem->exists(dest+".1"));
        contents = m_filesystem->readFile(dest+".1");
        EXPECT_EQ(contents,"Upgrade");

        ASSERT_FALSE(m_filesystem->exists(dest0));
    }
}
