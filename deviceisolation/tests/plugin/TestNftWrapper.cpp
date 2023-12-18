// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/XmlUtilities/AttributesMap.h"

#include "base/tests/Common/Helpers/MemoryAppender.h"

#include "pluginimpl/config.h"
#include "deviceisolation/modules/pluginimpl/NftWrapper.h"

#include <gtest/gtest.h>
#include <include/gmock/gmock-matchers.h>

#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include "tests/Common/Helpers/MockProcess.h"
#include "base/modules/Common/ProcessImpl/ProcessImpl.h"

namespace
{
    class TestNftWrapper : public MemoryAppenderUsingTests
    {
    public:
        TestNftWrapper() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {}
    };

    constexpr auto NFT_BINARY = "/opt/sophos-spl/plugins/deviceisolation/bin/nft";
    constexpr auto RULES_FILE = "/opt/sophos-spl/plugins/deviceisolation/var/nft_rules";
}

using namespace Plugin;

TEST_F(TestNftWrapper, applyIsolateRulesDefaultRuleset)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, _,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> args = {"-c", "-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));


                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::SUCCESS);
}


TEST_F(TestNftWrapper, applyIsolateRulesHandlesMissingNftBinary)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::applyIsolateRules({}));
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}

TEST_F(TestNftWrapper, applyIsolateRulesHandlesNftBinaryThatIsNotExecutable)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::applyIsolateRules({}));
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}

TEST_F(TestNftWrapper, applyIsolateRulesTimesOutIfNftHangs)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, _,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> args = {"-c", "-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}


TEST_F(TestNftWrapper, applyIsolateRulesHandlesNftFailure)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, _,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> args = {"-c", "-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(123));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("nft failed"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}

TEST_F(TestNftWrapper, clearIsolateRulesSucceeds)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(0));

                std::vector<std::string> args = { "flush", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);

                args = { "delete", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::SUCCESS);
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesMissingBinary)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}


TEST_F(TestNftWrapper, clearIsolateRulesHandlesNftBinaryThatIsNotExecutable)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}

TEST_F(TestNftWrapper, clearIsolateRulesTimesOutIfNftHangs)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                std::vector<std::string> args = { "flush", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}
