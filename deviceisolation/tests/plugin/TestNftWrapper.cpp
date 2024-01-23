// Copyright 2023-2024 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "pluginimpl/ApplicationPaths.h"
#include "pluginimpl/NTPPolicy.h"
#include "pluginimpl/config.h"
#include "pluginimpl/NftWrapper.h"

#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include "Common/Helpers/MemoryAppender.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockProcess.h"

#include <include/gmock/gmock-matchers.h>
#include <gtest/gtest.h>

namespace
{
    class TestNftWrapper : public MemoryAppenderUsingTests
    {
    public:
        TestNftWrapper() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {
        }

        void expectGroupLookup(gid_t gid=1)
        {
            auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
            EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid));
            scopeFilePermissions_ = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::move(mockFilePermissions));
        }

        std::unique_ptr<Tests::ScopedReplaceFilePermissions> scopeFilePermissions_;

        static constexpr auto NFT_TIMEOUT_DS = Plugin::NftWrapper::NFT_TIMEOUT_DS;
    };

    constexpr const auto* NFT_BINARY = "/opt/sophos-spl/plugins/deviceisolation/bin/nft";
    constexpr const auto* RULES_FILE = "/opt/sophos-spl/plugins/deviceisolation/var/nft_rules.conf";
}

using namespace Plugin;

TEST_F(TestNftWrapper, applyIsolateRulesDefaultRuleset)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    gid_t testGid = 123;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, HasSubstr("meta skgid " + std::to_string(testGid) + " accept"),
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(testGid);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));

                args = {"-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::SUCCESS);
    EXPECT_TRUE(appenderContains("Successfully set network filtering rules"));
}


TEST_F(TestNftWrapper, applyIsolateRulesHandlesMissingNftBinary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.applyIsolateRules({}));
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("nft binary does not exist: /opt/sophos-spl/plugins/deviceisolation/bin/nft"));
}

TEST_F(TestNftWrapper, applyIsolateRulesHandlesNftBinaryThatIsNotExecutable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.applyIsolateRules({}));
    EXPECT_EQ(result, IsolateResult::FAILED);

    EXPECT_TRUE(appenderContains("nft binary is not executable"));
}

TEST_F(TestNftWrapper, applyIsolateRulesTimesOutIfNftHangsListTable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, _,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft list table command did not complete in time, killing process"));
}

TEST_F(TestNftWrapper, applyIsolateRulesTimesOutIfNftReadingFromRulesFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, _,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> listargs = {"list", "table", "inet", "sophos_device_isolation"};
                std::vector<std::string> readargs = {"-f", Plugin::networkRulesFile()};

                InSequence s;
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, listargs)).Times(1).RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED)).RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1)).RetiresOnSaturation();

                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, readargs)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft command did not complete in time, killing process"));
}

TEST_F(TestNftWrapper, applyIsolateRulesHandlesNonZeroReturnFromNFTSettingRules)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(1);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> listargs = {"list", "table", "inet", "sophos_device_isolation"};
                std::vector<std::string> readargs = {"-f", Plugin::networkRulesFile()};

                EXPECT_CALL(*mockProcess, wait(_, _)).Times(2)
                    .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));

                InSequence s;
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, listargs)).Times(1).RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1)).RetiresOnSaturation();

                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, readargs)).Times(1);
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("process failure output"));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("Failed to set network rules, nft exit code: 1"));
    EXPECT_TRUE(appenderContains("nft output for set network: process failure output"));
}

TEST_F(TestNftWrapper, applyIsolateRulesHandlesRulesAlreadyExisting)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(1);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> listargs = {"list", "table", "inet", "sophos_device_isolation"};
                std::vector<std::string> readargs = {"-f", Plugin::networkRulesFile()};

                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, listargs)).Times(1);
                EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("table exists already"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::RULES_ALREADY_PRESENT);
    EXPECT_TRUE(appenderContains("nft list table output while applying rules: table exists already"));
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

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(123));

                args = {"-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(123));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("nft failed"));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules({});
    EXPECT_EQ(result, IsolateResult::FAILED);
}

TEST_F(TestNftWrapper, applyIsolateRulesWithExclusions)
{
    // Note that this policy has elements that don't really make sense, such as:
    //				<exclusion type="ip">
    //					<remoteAddress>100.78.0.42</remoteAddress>
    //					<remotePort>5671</remotePort>
    //				</exclusion>
    // which does not have a direction specified, so we have to add both. In reality the
    // user probably wants this to be an outgoing rule - but we have to deal with it as it
    // can be set, so that's why they're included here.
    constexpr const auto policyXml = R"SOPHOS(
<policy
	xmlns:csc="com.sophos\msys\csc">
	<csc:Comp policyType="24" RevID="fb3fb6e2889efd2e694ab1c64b0c488f0c09b29017835676802a19da94cecb15" />
	<configuration>
		<enabled>true</enabled>
		<connectionTracking>true</connectionTracking>
		<exclusions>
			<filePathSet>
				<filePath>/tmp/eicar.com</filePath>
				<filePath>/test1/</filePath>
			</filePathSet>
		</exclusions>
		<selfIsolation>
			<enabled>false</enabled>
			<exclusions>
				<exclusion type="ip">
					<direction>in</direction>
					<localPort>443</localPort>
				</exclusion>
				<exclusion type="ip">
					<direction>out</direction>
					<remoteAddress>192.168.1.9</remoteAddress>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>192.168.1.1</remoteAddress>
					<localPort>22</localPort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.42</remoteAddress>
					<remotePort>5671</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.43</remoteAddress>
					<remotePort>5671</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.44</remoteAddress>
					<remotePort>5671</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.11</remoteAddress>
					<remotePort>443</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.45</remoteAddress>
					<remotePort>443</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>100.78.0.46</remoteAddress>
					<remotePort>443</remotePort>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>2a00:1450:4009:823::200e</remoteAddress>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>2a00::/32</remoteAddress>
				</exclusion>
				<exclusion type="ip">
					<remoteAddress>8.8.8.8/16</remoteAddress>
				</exclusion>
			</exclusions>
		</selfIsolation>
		<ips>
			<enabled>false</enabled>
			<exclusions />
		</ips>
	</configuration>
</policy>)SOPHOS";
    NTPPolicy policy{policyXml};
    EXPECT_EQ(policy.revId(), "fb3fb6e2889efd2e694ab1c64b0c488f0c09b29017835676802a19da94cecb15");

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    const mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);

    const auto expectedRulesContents = NftWrapper::createIsolateRules(policy.exclusions(), 1);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, expectedRulesContents,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    expectGroupLookup(1);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));

                args = {"-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));


                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.applyIsolateRules(policy.exclusions());
    EXPECT_EQ(result, IsolateResult::SUCCESS);
}

TEST_F(TestNftWrapper, clearIsolateRulesSucceeds)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(0));

                std::vector<std::string> args = { "list", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);

                args = { "flush", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);

                args = { "delete", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.clearIsolateRules();
    EXPECT_EQ(result, IsolateResult::SUCCESS);
    EXPECT_TRUE(appenderContains("Successfully cleared Sophos network filtering rules"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesMissingBinary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("nft binary does not exist: /opt/sophos-spl/plugins/deviceisolation/bin/nft"));
}


TEST_F(TestNftWrapper, clearIsolateRulesHandlesNftBinaryThatIsNotExecutable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("nft binary is not executable"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesTableExistReturningNonZero)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, _)).Times(1);
                EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("process failure output"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::RULES_NOT_PRESENT);
    EXPECT_TRUE(appenderContains("nft exit code 1: rules probably not present"));
    EXPECT_TRUE(appenderContains("nft output for list table: process failure output"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesTableExistReturningNonOne)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, _)).Times(1);
                EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("process failure output 2"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    IsolateResult result;
    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::RULES_NOT_PRESENT);
    EXPECT_TRUE(appenderContains("Failed to list table, nft exit code: 2"));
    EXPECT_TRUE(appenderContains("nft output for list table: process failure output 2"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesFlushTableReturningNonZero)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, _)).Times(2);
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(2)
                            .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0)).RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("process failure output"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    IsolateResult result;

    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("Failed to flush table, nft exit code: 1"));
    EXPECT_TRUE(appenderContains("nft output for flush table: process failure output"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesDeleteTableReturningNonZero)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(_, _)).Times(3);
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(3)
                            .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
                EXPECT_CALL(*mockProcess, exitCode()).Times(2)
                            .WillRepeatedly(Return(0)).RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return("process failure output"));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });


    IsolateResult result;

    auto nftWrapper = NftWrapper();
    EXPECT_NO_THROW(result = nftWrapper.clearIsolateRules());
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("Failed to delete table, nft exit code: 1"));
    EXPECT_TRUE(appenderContains("nft output for delete table: process failure output"));
}

TEST_F(TestNftWrapper, clearIsolateRulesTimesOutIfNftHangsCheckingIfTableExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                std::vector<std::string> args = { "list", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.clearIsolateRules();
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft list table command did not complete in time"));
}

TEST_F(TestNftWrapper, clearIsolateRulesTimesOutIfNftHangsFlushingTable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                std::vector<std::string> listargs = { "list", "table", "inet", "sophos_device_isolation" };
                std::vector<std::string> flushargs = { "flush", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, listargs)).Times(1);
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, flushargs)).Times(1);
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(1)
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED))
                        .RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.clearIsolateRules();
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft flush table command did not complete in time"));
}

TEST_F(TestNftWrapper, clearIsolateRulesTimesOutIfNftHangsDeletingTable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> listargs = { "list", "table", "inet", "sophos_device_isolation" };
                std::vector<std::string> flushargs = { "flush", "table", "inet", "sophos_device_isolation" };
                std::vector<std::string> deleteargs = { "delete", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, listargs)).Times(1);
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, flushargs)).Times(1);
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, deleteargs)).Times(1);
                EXPECT_CALL(*mockProcess, exitCode()).Times(2).WillRepeatedly(Return(0));
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), NFT_TIMEOUT_DS))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(2)
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED))
                        .RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto nftWrapper = NftWrapper();
    IsolateResult result = nftWrapper.clearIsolateRules();
    EXPECT_EQ(result, IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft delete table command did not complete in time"));
}

TEST_F(TestNftWrapper, createRulesWithNoExclusions)
{
    expectGroupLookup(1);

    auto rules = NftWrapper::createIsolateRules({});
}

namespace
{
    using Plugin::IsolationExclusion;
    static IsolationExclusion createExclusion(
            IsolationExclusion::Direction direction,
            IsolationExclusion::port_list_t localPort
            )
    {
        Plugin::IsolationExclusion excl;
        excl.setDirection(direction);
        excl.setLocalPorts(std::move(localPort));
        return excl;
    }
    static IsolationExclusion createExclusion(
            IsolationExclusion::Direction direction,
            IsolationExclusion::address_iptype_list_t addresses
    )
    {
        Plugin::IsolationExclusion excl;
        excl.setDirection(direction);
        excl.setRemoteAddressesAndIpTypes(std::move(addresses));
        return excl;
    }
    static IsolationExclusion createExclusion(
            IsolationExclusion::port_list_t localPort,
            IsolationExclusion::address_iptype_list_t addresses
    )
    {
        Plugin::IsolationExclusion excl;
        excl.setLocalPorts(std::move(localPort));
        excl.setRemoteAddressesAndIpTypes(std::move(addresses));
        return excl;
    }

    static IsolationExclusion createExclusion(
            IsolationExclusion::address_iptype_list_t addresses,
            IsolationExclusion::port_list_t remotePorts
    )
    {
        Plugin::IsolationExclusion excl;
        excl.setRemoteAddressesAndIpTypes(std::move(addresses));
        excl.setRemotePorts(std::move(remotePorts));
        return excl;
    }
    static IsolationExclusion createExclusion(
            IsolationExclusion::address_iptype_list_t addresses
    )
    {
        Plugin::IsolationExclusion excl;
        excl.setRemoteAddressesAndIpTypes(std::move(addresses));
        return excl;
    }
}

TEST_F(TestNftWrapper, createRulesWithInAllowPort)
{
    using Plugin::IsolationExclusion;
    std::vector<IsolationExclusion> allowList;
    allowList.push_back(createExclusion(IsolationExclusion::Direction::IN, {"443"}));

    auto actualRulesContents = NftWrapper::createIsolateRules(allowList, 1);

    EXPECT_THAT(actualRulesContents, HasSubstr("tcp dport 443 accept"));
    EXPECT_THAT(actualRulesContents, HasSubstr("udp dport 443 accept"));
}

TEST_F(TestNftWrapper, createRulesWithManyExclusions)
{
    using Plugin::IsolationExclusion;
    using address_iptype_list_t = Plugin::IsolationExclusion::address_iptype_list_t;
    std::vector<IsolationExclusion> allowList;
    allowList.push_back(createExclusion(IsolationExclusion::Direction::IN, {"443"}));
    allowList.push_back(createExclusion(IsolationExclusion::Direction::OUT, address_iptype_list_t{{"192.168.1.9", "ip"}}));
    allowList.push_back(createExclusion({"22"}, address_iptype_list_t{{"192.168.1.1", "ip"}}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.42", "ip"}}, {"5671"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.43", "ip"}}, {"5671"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.44", "ip"}}, {"5671"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.11", "ip"}}, {"443"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.45", "ip"}}, {"443"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"100.78.0.46", "ip"}}, {"443"}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"2a00:1450:4009:823::200e", "ip6"}}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"2a00::/32", "ip6"}}));
    allowList.push_back(createExclusion(address_iptype_list_t{{"8.8.8.8/16", "ip"}}));

    auto actualRulesContents = NftWrapper::createIsolateRules(allowList, 1);

    const std::string expectedRulesContents = R"SOPHOS(table inet sophos_device_isolation {
    chain INPUT {
            type filter hook input priority filter; policy drop;
            ct state invalid drop
            iif "lo" accept
            ct state established,related accept
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept
            ip6 version 6 udp dport 546 accept
            ip version 4 udp dport 68 accept
            tcp dport 443 accept
            udp dport 443 accept
            ip saddr 192.168.1.1 tcp dport 22 accept
            ip saddr 192.168.1.1 udp dport 22 accept
            ip saddr 100.78.0.42 tcp sport 5671 accept
            ip saddr 100.78.0.42 udp sport 5671 accept
            ip saddr 100.78.0.43 tcp sport 5671 accept
            ip saddr 100.78.0.43 udp sport 5671 accept
            ip saddr 100.78.0.44 tcp sport 5671 accept
            ip saddr 100.78.0.44 udp sport 5671 accept
            ip saddr 100.78.0.11 tcp sport 443 accept
            ip saddr 100.78.0.11 udp sport 443 accept
            ip saddr 100.78.0.45 tcp sport 443 accept
            ip saddr 100.78.0.45 udp sport 443 accept
            ip saddr 100.78.0.46 tcp sport 443 accept
            ip saddr 100.78.0.46 udp sport 443 accept
            ip6 saddr 2a00:1450:4009:823::200e accept
            ip6 saddr 2a00::/32 accept
            ip saddr 8.8.8.8/16 accept

    }

    chain FORWARD {
            type filter hook forward priority filter; policy drop;
    }

    chain OUTPUT {
            type filter hook output priority filter; policy drop;
            oif "lo" accept
            ct state established,related accept
            tcp dport 53 accept
            udp dport 53 accept
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept
            ip version 4 udp dport 67 accept
            ip6 version 6 udp dport 547 accept
            ip daddr 192.168.1.9 accept
            ip daddr 192.168.1.1 tcp sport 22 accept
            ip daddr 192.168.1.1 udp sport 22 accept
            ip daddr 100.78.0.42 tcp dport 5671 accept
            ip daddr 100.78.0.42 udp dport 5671 accept
            ip daddr 100.78.0.43 tcp dport 5671 accept
            ip daddr 100.78.0.43 udp dport 5671 accept
            ip daddr 100.78.0.44 tcp dport 5671 accept
            ip daddr 100.78.0.44 udp dport 5671 accept
            ip daddr 100.78.0.11 tcp dport 443 accept
            ip daddr 100.78.0.11 udp dport 443 accept
            ip daddr 100.78.0.45 tcp dport 443 accept
            ip daddr 100.78.0.45 udp dport 443 accept
            ip daddr 100.78.0.46 tcp dport 443 accept
            ip daddr 100.78.0.46 udp dport 443 accept
            ip6 daddr 2a00:1450:4009:823::200e accept
            ip6 daddr 2a00::/32 accept
            ip daddr 8.8.8.8/16 accept
            meta skgid 1 accept
            reject with tcp reset
    }
})SOPHOS";
    EXPECT_EQ(actualRulesContents, expectedRulesContents);
}