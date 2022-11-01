// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "unixsocket/threatDetectorSocket/ThreatDetectedMessageUtils.h"

#include "tests/common/LogInitializedTests.h"
#include "tests/datatypes/MockSysCalls.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class TestThreatDetectedMessageUtils : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };
}

TEST_F(TestThreatDetectedMessageUtils, readCapnProtoMsg_returnsFalseWithNegativeReadRetCode) //NOLINT
{
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);
    int32_t length = 1;
    EXPECT_CALL(*m_mockSysCalls, read(_, proto_buffer.begin(), length)).WillOnce(SetErrnoAndReturn(ENOENT, -1));

    ssize_t bytes_read;
    bool loggedLengthOfZero = false;
    std::string errMsg;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_FALSE(unixsocket::readCapnProtoMsg(m_mockSysCalls, length, buffer_size, proto_buffer, fdHolder, bytes_read, loggedLengthOfZero, errMsg));
    EXPECT_EQ(errMsg, "Aborting Scanning connection thread: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, readCapnProtoMsg_returnsFalseWithUnexpectedLengthReturnedByRead) //NOLINT
{
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);
    int32_t length = 1;
    EXPECT_CALL(*m_mockSysCalls, read(_, proto_buffer.begin(), length)).WillOnce(Return(length+1));

    ssize_t bytes_read;
    bool loggedLengthOfZero = false;
    std::string errMsg;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_FALSE(unixsocket::readCapnProtoMsg(m_mockSysCalls, length, buffer_size, proto_buffer, fdHolder, bytes_read, loggedLengthOfZero, errMsg));
    EXPECT_EQ(errMsg, "Aborting Scanning connection thread: failed to read entire message");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFdFile_returnsFalseWithNegativeFstatRetCode) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_CALL(*m_mockSysCalls, fstat(_, _)).WillOnce(SetErrnoAndReturn(ENOENT, -1));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFdFile(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Failed to get status of received file FD: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFdFile_returnsFalseIfNotRegularFile) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    struct ::stat st{};
    st.st_mode = S_IFLNK;
    EXPECT_CALL(*m_mockSysCalls, fstat(_, _)).WillOnce(DoAll(SetArgPointee<1>(st), Return(0)));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFdFile(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Received file FD is not a regular file");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFileOpen_returnsFalseWithNegativeFcntlRetCode) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_CALL(*m_mockSysCalls, fcntl(_, F_GETFL)).WillOnce(SetErrnoAndReturn(ENOENT, -1));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFileOpen(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Failed to get status flags of received file FD: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFileOpen_returnsFalseIfFileNotOpen) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    int status = O_WRONLY;
    EXPECT_CALL(*m_mockSysCalls, fcntl(_, F_GETFL)).WillOnce(Return(status));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFileOpen(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Received file FD is not open for read");
}