/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProcUtilImpl/ProcUtilities.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessInfo.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <fstream>


using namespace Common::Process;
namespace
{
    class ProcUtilImplTests : public LogInitializedTests
    {
    public:
        ProcUtilImplTests()
        {
            mockIFileSystemPtr = new StrictMock<MockFileSystem>();
            m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockIFileSystemPtr));
        }
        MockFileSystem* mockIFileSystemPtr;
        Tests::ScopedReplaceFileSystem m_replacer;

        std::optional<std::string> procStatusContent(int pid, int uid)
        {
            std::stringstream content;
            content << "Pid:\t" << pid << std::endl;
            content << "PPid:\t1" << std::endl;
            content << "TracerPid:\t0" << std::endl;
            content << "Uid:\t" << uid << "\t" << uid << "\t" << uid << "\t" << uid << std::endl;
            content << "Gid:\t124\t124\t124\t124" << std::endl;
            content << "FDSize: 64" << std::endl;
            return content.str();
        }
    };

    TEST_F(ProcUtilImplTests, testParseProcStatRetrievesCorrectValues) // NOLINT
    {
        std::string statContents = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat = Proc::parseProcStat(statContents);
        EXPECT_TRUE(procStat.has_value());
        EXPECT_EQ(procStat.value().pid,21);
        EXPECT_EQ(procStat.value().comm,"(migration/2)");
        EXPECT_EQ(procStat.value().state,Proc::ProcStat::ProcState::Active);
        EXPECT_EQ(procStat.value().ppid,2);
    }

    TEST_F(ProcUtilImplTests, testParseProcStatReturnsEmptyOptionalOnBadProcFile) // NOLINT
    {
        // stat file is shorter than it should be
        std::string statContents = "21 (migration/2)";
        std::optional<Proc::ProcStat> procStat = Proc::parseProcStat(statContents);
        EXPECT_FALSE(procStat.has_value());
    }

    TEST_F(ProcUtilImplTests, testParseProcStatRetrievesCorrectStateOfProcess) // NOLINT
    {
        std::string statContents1 = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::string statContents2 = "21 (migration/2) Z 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::string statContents3 = "21 (migration/2) X 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::string statContents4 = "21 (migration/2) x 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat1 = Proc::parseProcStat(statContents1);
        std::optional<Proc::ProcStat> procStat2 = Proc::parseProcStat(statContents2);
        std::optional<Proc::ProcStat> procStat3 = Proc::parseProcStat(statContents3);
        std::optional<Proc::ProcStat> procStat4 = Proc::parseProcStat(statContents4);
        EXPECT_EQ(procStat1.value().state,Proc::ProcStat::ProcState::Active);
        EXPECT_EQ(procStat2.value().state,Proc::ProcStat::ProcState::Finished);
        EXPECT_EQ(procStat3.value().state,Proc::ProcStat::ProcState::Finished);
        EXPECT_EQ(procStat4.value().state,Proc::ProcStat::ProcState::Finished);
    }

    TEST_F(ProcUtilImplTests, testParseProcStatWithInvalidFieldsReturnsEmptyOptional) // NOLINT
    {
        // twentyone is not an int
        std::string statContents = "twentyone (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat = Proc::parseProcStat(statContents);
        EXPECT_FALSE(procStat.has_value());

        // two is not an int
        std::string statContents2 = "21 (migration/2) S two 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat2 = Proc::parseProcStat(statContents2);
        EXPECT_FALSE(procStat2.has_value());

        // state entry is single char, SS is too long
        std::string statContents3 = "21 (migration/2) SS 21 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat3 = Proc::parseProcStat(statContents3);
        EXPECT_FALSE(procStat3.has_value());

        // 9 as state entry is not a valid state
        std::string statContents4 = "21 (migration/2) 9 21 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
        std::optional<Proc::ProcStat> procStat4 = Proc::parseProcStat(statContents4);
        EXPECT_FALSE(procStat4.has_value());
    }


    TEST_F(ProcUtilImplTests, testgetUserIdFromStatusRetrievesCorrectUserId) // NOLINT
    {
        int pid = 24;
        int uid = 119;
        std::string processFileType = "status";
        std::optional<std::string> goodStatusFileContents = procStatusContent(pid,uid);

        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(pid, processFileType)).WillOnce(Return(goodStatusFileContents));
        EXPECT_EQ(Proc::getUserIdFromStatus(pid),uid);

        int pid2 = 25;
        int uid2= 2341;
        std::optional<std::string> goodStatusFileContents2 = procStatusContent(pid2,uid2);

        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(pid2, processFileType)).WillOnce(Return(goodStatusFileContents2));
        EXPECT_EQ(Proc::getUserIdFromStatus(pid2),uid2);
    }


    TEST_F(ProcUtilImplTests, testgetUserIdFromStatusHandlesInvalidFormat) // NOLINT
    {
        //bad format of UserId
        int pid = 26;
        std::string processFileType = "status";

        std::stringstream content;
        content << "Pid:\t26" << std::endl;
        content << "PPid:\t1" << std::endl;
        content << "TracerPid:\t0" << std::endl;
        content << "Uid:\tRANDOM\tTEXT\tHERE\tBAD" << std::endl;
        content << "Gid:\t124\t124\t124\t124" << std::endl;
        content << "FDSize: 64" << std::endl;
        std::optional<std::string> goodStatusFileContents = content.str();

        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(pid, processFileType)).WillOnce(Return(goodStatusFileContents));
        EXPECT_EQ(Proc::getUserIdFromStatus(26),-1);

        //no UserId inside the file
        int pid2 = 27;

        std::stringstream content2;
        content2 << "Pid:\t27" << std::endl;
        content2 <<  "PPid:\t1" << std::endl;
        content2 << "TracerPid:\t0" << std::endl;
        content2 << "Gid:\t124\t124\t124\t124" << std::endl;
        content2 << "FDSize: 64" << std::endl;
        std::optional<std::string> goodStatusFileContents2 = content2.str();

        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(pid2, processFileType)).WillOnce(Return(goodStatusFileContents2));
        EXPECT_EQ(Proc::getUserIdFromStatus(27),-1);
    }

    TEST_F(ProcUtilImplTests, testListProcWithUserNameRetrievesCorrectPids) // NOLINT
    {
        std::vector<Path> mockPaths = {
            "/proc/21",
            "/proc/22",
            "/proc/23",
            "/proc/24",
            "/proc/25",
            "/proc/a",
            "/proc/bb",
            "/proc/ccc",
            "/proc/abc"
        };

        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

        std::string processFileType = "status";
        std::string testUser = "testUser";
        int testUserUserId= 112;
        int otherUserId= 1234;
        int randomPid= 122; //not relevant as we only parse the UserId from the Status file
        std::optional<std::string> statusFileWithUserIdOfTestUser = procStatusContent(randomPid,testUserUserId);
        std::optional<std::string> statusFileWithOtherUserId = procStatusContent(randomPid,otherUserId);

        EXPECT_CALL(*mockFilePermissions, getUserId(testUser)).WillOnce(Return(testUserUserId));
        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(21, processFileType)).WillOnce(Return(statusFileWithUserIdOfTestUser));
        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(22, processFileType)).WillOnce(Return(statusFileWithOtherUserId));
        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(23, processFileType)).WillOnce(Return(statusFileWithOtherUserId));
        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(24, processFileType)).WillOnce(Return(statusFileWithUserIdOfTestUser));
        EXPECT_CALL(*mockIFileSystemPtr, readProcFile(25, processFileType)).WillOnce(Return(statusFileWithOtherUserId));
        EXPECT_CALL(*mockIFileSystemPtr, listFilesAndDirectories("/proc",false)).WillOnce(Return(mockPaths));
        EXPECT_TRUE(Proc::listProcWithUserName(testUser).size()==2);

    }








} // namespace
