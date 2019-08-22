/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPidLockFileUtils.h"

#include <Common/OSUtilitiesImpl/PidLockFile.h>
#include <tests/Common/Helpers/TempDir.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <future>

using PidLockFile = Common::OSUtilitiesImpl::PidLockFile;

class TestPidLockFile : public ::testing::Test
{
public:
    TestPidLockFile()
    {
        std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils(new StrictMock<MockPidLockFileUtils>());
        m_mockPidLockFileUtilsPtr = mockPidLockFileUtils.get();
        Common::OSUtilitiesImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));
    }
    ~TestPidLockFile() { Common::OSUtilitiesImpl::restorePidLockUtils(); }
    MockPidLockFileUtils* m_mockPidLockFileUtilsPtr;
};

TEST_F(TestPidLockFile, pidLockFileThrowsWithBadFileDescriptor) // NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(-1));
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error); // NOLINT
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToLockFile) // NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error); // NOLINT
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToTruncateFile) // NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error); // NOLINT
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToWriteFile) // NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, getpid()).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, write(_, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error); // NOLINT
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfItDoesNotWriteCorrectNumberOfBytes) // NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(0));

    // "10" uses two bytes, pretend we only write 1
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, getpid()).WillOnce(Return(10));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, write(_, _, _)).WillOnce(Return(1));

    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error); // NOLINT
}

TEST_F(TestPidLockFile, pidLockFileDoesntThrowIfSuccessful) // NOLINT
{
    std::string pidFilePath("/bogus/testpath/fake.pid");
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(pidFilePath, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, getpid()).WillOnce(Return(100));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, write(_, _, _)).WillOnce(Return(3));

    // Destructor
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(0)).WillOnce(Return());
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, unlink(pidFilePath)).WillOnce(Return());

    EXPECT_NO_THROW(PidLockFile pidfile(pidFilePath)); // NOLINT
}

TEST( TestLockFile, aquireLockFileShouldAllowOnlyOneHolder )
{
    Tests::TempDir tempDir;
    std::string lockfile = tempDir.absPath("my.lock");
    auto holder = Common::OSUtilities::acquireLockFile(lockfile);
    ASSERT_TRUE(holder);
    auto lockfunctor = [&lockfile]() {
        try
        {
            auto q = Common::OSUtilities::acquireLockFile(lockfile);
            return 0;
        }
        catch (std::system_error&)
        {
            return 1;
        }
    };
    auto fut = std::async(std::launch::async,lockfunctor);
    EXPECT_EQ( fut.get(), 1);
    
    EXPECT_THROW(Common::OSUtilities::acquireLockFile(lockfile), std::system_error);
    
    holder.reset();
    ASSERT_FALSE(holder);
    auto fut2 = std::async(std::launch::async, lockfunctor);
    EXPECT_EQ( fut2.get(), 0);
}
