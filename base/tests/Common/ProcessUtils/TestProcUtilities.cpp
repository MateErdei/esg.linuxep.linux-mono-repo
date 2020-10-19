/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ProcessUtils/ProcUtilities.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

using namespace ::testing;

class ProcTests : public LogInitializedTests
{
public:
    ProcTests()
    {
        mockIFileSystemPtr = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockIFileSystemPtr));
    }
    MockFileSystem* mockIFileSystemPtr;
    Tests::ScopedReplaceFileSystem m_replacer;

};

TEST_F(ProcTests, testReadProcFile) // NOLINT
{
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/1/exists")).WillOnce(Return("content"));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/1/doesNotExist")).WillOnce(Throw(IFileSystemException("file does not exist")));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/2/alsoExists")).WillOnce(Return("more content"));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/2/doesNotExist")).WillOnce(Throw(IFileSystemException("file still does not exist")));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/3/directory/exists")).WillOnce(Return("different content"));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/9999999/exists")).WillOnce(Return("bigger pid content"));

    std::optional<std::string> a = ProcessUtils::readProcFile(1,"exists");
    EXPECT_TRUE(a.has_value() && a.value() == "content");
    std::optional<std::string> b = ProcessUtils::readProcFile(1,"doesNotExist");
    EXPECT_FALSE(b.has_value());
    std::optional<std::string> c = ProcessUtils::readProcFile(2,"alsoExists");
    EXPECT_TRUE(c.has_value() && c.value() == "more content");
    std::optional<std::string> d = ProcessUtils::readProcFile(2,"doesNotExist");
    EXPECT_FALSE(d.has_value());
    std::optional<std::string> e = ProcessUtils::readProcFile(3,"directory/exists");
    EXPECT_TRUE(e.has_value() && e.value() == "different content");
    std::optional<std::string> f = ProcessUtils::readProcFile(9999999,"exists");
    EXPECT_TRUE(f.has_value() && f.value() == "bigger pid content");
}

TEST_F(ProcTests, testParseProcStat) // NOLINT
{
    std::string statContents = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat = ProcessUtils::parseProcStat(statContents);
    EXPECT_TRUE(procStat.has_value());
    EXPECT_EQ(procStat.value().pid,21);
    EXPECT_EQ(procStat.value().comm,"(migration/2)");
    EXPECT_EQ(procStat.value().state,ProcessUtils::ProcStat::ProcState::Active);
    EXPECT_EQ(procStat.value().ppid,2);
}

TEST_F(ProcTests, testParseStatThatWentWrong) // NOLINT
{
    // stat file is shorter than it should be
    std::string statContents = "21 (migration/2)";
    std::optional<ProcessUtils::ProcStat> procStat = ProcessUtils::parseProcStat(statContents);
    EXPECT_FALSE(procStat.has_value());
}

TEST_F(ProcTests, testInactiveStateFieldIsTranslatedToInactive) // NOLINT
{
    std::string statContents1 = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::string statContents2 = "21 (migration/2) Z 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::string statContents3 = "21 (migration/2) X 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::string statContents4 = "21 (migration/2) x 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat1 = ProcessUtils::parseProcStat(statContents1);
    std::optional<ProcessUtils::ProcStat> procStat2 = ProcessUtils::parseProcStat(statContents2);
    std::optional<ProcessUtils::ProcStat> procStat3 = ProcessUtils::parseProcStat(statContents3);
    std::optional<ProcessUtils::ProcStat> procStat4 = ProcessUtils::parseProcStat(statContents4);
    EXPECT_EQ(procStat1.value().state,ProcessUtils::ProcStat::ProcState::Active);
    EXPECT_EQ(procStat2.value().state,ProcessUtils::ProcStat::ProcState::Finished);
    EXPECT_EQ(procStat3.value().state,ProcessUtils::ProcStat::ProcState::Finished);
    EXPECT_EQ(procStat4.value().state,ProcessUtils::ProcStat::ProcState::Finished);
}

TEST_F(ProcTests, testParseProcStatWithInvalidFieldsReturnsEmptyOptional) // NOLINT
{
    // twentyone is not an int
    std::string statContents = "twentyone (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat = ProcessUtils::parseProcStat(statContents);
    EXPECT_FALSE(procStat.has_value());

    // two is not an int
    std::string statContents2 = "21 (migration/2) S two 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat2 = ProcessUtils::parseProcStat(statContents2);
    EXPECT_FALSE(procStat2.has_value());

    // state entry is single char, SS is too long
    std::string statContents3 = "21 (migration/2) SS 21 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat3 = ProcessUtils::parseProcStat(statContents3);
    EXPECT_FALSE(procStat3.has_value());

    // 9 as state entry is not a valid state
    std::string statContents4 = "21 (migration/2) 9 21 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    std::optional<ProcessUtils::ProcStat> procStat4 = ProcessUtils::parseProcStat(statContents4);
    EXPECT_FALSE(procStat4.has_value());
}


TEST_F(ProcTests, testListAndFilterProcessesAppendsWhenWhenFilterIsTrue)
{
    std::vector<Path> mockPaths = {"/proc/21"};
    std::string goodStatFileContents = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/21/stat")).Times(2).WillRepeatedly(Return(goodStatFileContents));
    EXPECT_CALL(*mockIFileSystemPtr, listFilesAndDirectories("/proc",false)).Times(2).WillRepeatedly(Return(mockPaths));
    auto alwaysTrue = [](const ProcessUtils::ProcStat&) {return true;};
    auto alwaysFalse = [](const ProcessUtils::ProcStat&) {return false;};
    EXPECT_FALSE(ProcessUtils::listAndFilterProcesses(alwaysTrue).empty());
    EXPECT_TRUE(ProcessUtils::listAndFilterProcesses(alwaysFalse).empty());
}

TEST_F(ProcTests, testListAndFilterProcessesFiltersNonPidDirectoriesProperly)
{
    std::vector<Path> mockPaths = {
                                    "/proc/21",
                                    "/proc/22",
                                    "/proc/23",
                                    "/proc/24",
                                    "/proc/a",
                                    "/proc/bb",
                                    "/proc/ccc",
                                    "/proc/abc"
                                   };
    std::string goodStatFileContents = "21 (migration/2) S 2 0 0 0 -1 69238848 0 0 0 0 2 7 0 0 -100 0 1 0 2 0 0 18446744073709551615 0 0 0 0 0 0 0 2147483647 0 0 0 0 17 2 99 1 0 0 0 0 0 0 0 0 0 0 0";
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/21/stat")).WillOnce(Return(goodStatFileContents));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/22/stat")).WillOnce(Return(goodStatFileContents));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/23/stat")).WillOnce(Return(goodStatFileContents));
    EXPECT_CALL(*mockIFileSystemPtr, readProcStyleFile("/proc/24/stat")).WillOnce(Return(goodStatFileContents));
    EXPECT_CALL(*mockIFileSystemPtr, listFilesAndDirectories("/proc",false)).WillOnce(Return(mockPaths));
    auto alwaysTrue = [](const ProcessUtils::ProcStat&) {return true;};
    EXPECT_FALSE(ProcessUtils::listAndFilterProcesses(alwaysTrue).empty());
}

TEST_F(ProcTests, testFilterProcessesFiltersProperly)
{
    ProcessUtils::ProcStat process = {21,ProcessUtils::ProcStat::ProcState::Active, 2, "(migration/2)"};
    std::vector<ProcessUtils::ProcStat> processes = {process};
    auto alwaysTrue = [](const ProcessUtils::ProcStat&) {return true;};
    auto alwaysFalse = [](const ProcessUtils::ProcStat&) {return false;};
    EXPECT_EQ(ProcessUtils::filterProcesses(processes, alwaysTrue).size(), 1);
    EXPECT_TRUE(ProcessUtils::filterProcesses(processes, alwaysFalse).empty());
}