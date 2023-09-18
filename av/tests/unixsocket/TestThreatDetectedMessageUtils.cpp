// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "unixsocket/threatDetectorSocket/ThreatDetectedMessageUtils.h"

#include "tests/common/LogInitializedTests.h"

#include "Common/Helpers/MockSysCalls.h"

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

        bool readCapnProtoMsg(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCallWrapper,
            ssize_t length,
            uint32_t& buffer_size,
            kj::Array<capnp::word>& proto_buffer,
            datatypes::AutoFd& socket_fd,
            ssize_t& bytes_read,
            bool& loggedLengthOfZero,
            std::string& errMsg)
        {
            return unixsocket::readCapnProtoMsg(
                sysCallWrapper,
                length,
                buffer_size,
                proto_buffer,
                socket_fd,
                bytes_read,
                loggedLengthOfZero,
                errMsg,
                std::chrono::milliseconds{10}
            );
        }
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
    EXPECT_FALSE(readCapnProtoMsg(m_mockSysCalls, length, buffer_size, proto_buffer, fdHolder, bytes_read, loggedLengthOfZero, errMsg));
    EXPECT_EQ(errMsg, "Aborting: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, readCapnProtoMsg_returnsFalseWithUnexpectedLengthReturnedByRead)
{
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);
    int32_t length = 1;
    EXPECT_CALL(*m_mockSysCalls, read(_, proto_buffer.begin(), length)).WillOnce(Return(length+1));

    ssize_t bytes_read;
    bool loggedLengthOfZero = false;
    std::string errMsg;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_FALSE(readCapnProtoMsg(m_mockSysCalls, length, buffer_size, proto_buffer, fdHolder, bytes_read, loggedLengthOfZero, errMsg));
    EXPECT_EQ(errMsg, "Aborting: failed to read entire message");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFdFile_returnsFalseWithNegativeFstatRetCode)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_CALL(*m_mockSysCalls, fstat(_, _)).WillOnce(SetErrnoAndReturn(ENOENT, -1));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFdFile(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Failed to get status of received file FD: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFdFile_returnsFalseIfNotRegularFile)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    struct ::stat st{};
    st.st_mode = S_IFLNK;
    EXPECT_CALL(*m_mockSysCalls, fstat(_, _)).WillOnce(DoAll(SetArgPointee<1>(st), Return(0)));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFdFile(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Received file FD is not a regular file");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFileOpen_returnsFalseWithNegativeFcntlRetCode)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_CALL(*m_mockSysCalls, fcntl(_, F_GETFL)).WillOnce(SetErrnoAndReturn(ENOENT, -1));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFileOpen(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Failed to get status flags of received file FD: No such file or directory");
}

TEST_F(TestThreatDetectedMessageUtils, isReceivedFileOpen_returnsFalseIfFileNotOpen)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    int status = O_WRONLY;
    EXPECT_CALL(*m_mockSysCalls, fcntl(_, F_GETFL)).WillOnce(Return(status));

    std::string errMsg;
    EXPECT_FALSE(unixsocket::isReceivedFileOpen(m_mockSysCalls, fdHolder, errMsg));
    EXPECT_EQ(errMsg, "Received file FD is not open for read");
}