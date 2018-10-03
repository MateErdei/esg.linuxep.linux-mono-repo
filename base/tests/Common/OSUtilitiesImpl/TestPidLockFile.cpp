/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/#include <gtest/gtest.h>


#include "MockPidLockFileUtils.h"

#include <Common/OSUtilitiesImpl/PidLockFile.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using PidLockFile = Common::OSUtilitiesImpl::PidLockFile;

class TestPidLockFile : public ::testing::Test
{
public:

    TestPidLockFile()
    {
        std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils (new StrictMock<MockPidLockFileUtils>());
        m_mockPidLockFileUtilsPtr = mockPidLockFileUtils.get();
        Common::OSUtilitiesImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));
    }
    ~TestPidLockFile()
    {
        Common::OSUtilitiesImpl::restorePidLockUtils();
    }
    MockPidLockFileUtils * m_mockPidLockFileUtilsPtr;

};


TEST_F(TestPidLockFile, pidLockFileThrowsWithBadFileDescriptor) //NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(-1));
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error);
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToLockFile) //NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error);
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToTruncateFile) //NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error);
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfUnableToWriteFile) //NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, getpid()).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, write(_, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error);
}

TEST_F(TestPidLockFile, pidLockFileThrowsIfItDoesNotWriteCorrectNumberOfBytes) //NOLINT
{
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, open(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, lockf(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, ftruncate(_, _)).WillOnce(Return(0));

    // "10" uses two bytes, pretend we only write 1
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, getpid()).WillOnce(Return(10));
    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, write(_, _, _)).WillOnce(Return(1));

    EXPECT_CALL(*m_mockPidLockFileUtilsPtr, close(_)).WillOnce(Return());
    EXPECT_THROW(PidLockFile pidfile("/bogus/testpath/fake.pid"), std::system_error);
}

TEST_F(TestPidLockFile, pidLockFileDoesntThrowIfSuccessful) //NOLINT
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

    EXPECT_NO_THROW(PidLockFile pidfile(pidFilePath));
}