/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"

using namespace Common::FileSystem;
namespace
{

    class FileSystemImplTest : public ::testing::Test
    {
    public:
        std::unique_ptr<IFileSystem> m_fileSystem;
        void SetUp() override
        {
            m_fileSystem.reset(new FileSystemImpl());
        }
    };

    TEST_F( FileSystemImplTest, basenameReturnsCorrectMatchingValue)
    {
        std::vector<std::pair<std::string, std::string>> values = {
                {"/tmp/tmpfile.txt", "tmpfile.txt" },
                {"/tmp/tmpfile/", "" },
                {"/tmp/tmp1/tmp2/tmp3/tmpfile.txt", "tmpfile.txt" },
                {"/tmp", "tmp"},
                {"tmp", "tmp"},
                {"",""}
        };
        for ( auto & pair: values)
        {
            EXPECT_EQ(m_fileSystem->basename(pair.first), pair.second);
        }

    }



}

