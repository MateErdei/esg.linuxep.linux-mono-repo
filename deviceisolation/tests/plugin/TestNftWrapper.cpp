// Copyright 2023 Sophos Limited. All rights reserved.

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
        {}
    };

    constexpr auto NFT_BINARY = "/opt/sophos-spl/plugins/deviceisolation/bin/nft";
    constexpr auto RULES_FILE = "/opt/sophos-spl/plugins/deviceisolation/var/nft_rules.conf";
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

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(testGid));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));

                args = {"-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::SUCCESS);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::applyIsolateRules({}));
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::applyIsolateRules({}));
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);

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

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

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
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));

                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

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

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("Failed to set network rules, nft exit code: 1"));
    EXPECT_TRUE(appenderContains("nft output for set network: process failure output"));
}

TEST_F(TestNftWrapper, applyIsolateRulesHandlesNonZeroReturnFromNFTListRules)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, _, _, _)).Times(1);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

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

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules({});
    EXPECT_EQ(result, NftWrapper::IsolateResult::RULES_NOT_PRESENT);
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

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(123));

                args = {"-f", RULES_FILE};
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
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    std::string expectedRulesContents = R"SOPHOS(table inet sophos_device_isolation {
    chain INPUT {
            type filter hook input priority filter; policy drop;
            ct state invalid drop
            iifname "lo" accept
            jump states
            ip protocol icmp accept
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

    }

    chain FORWARD {
            type filter hook forward priority filter; policy drop;
    }

    chain OUTPUT {
            type filter hook output priority filter; policy drop;
            jump outgoing-services
            oifname "lo" accept
            jump states
            jump outgoing-services
    }

    chain outgoing-services {
            tcp dport 53 accept
            udp dport 53 accept
            ip protocol icmp accept
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
            meta skgid 1 accept

    }

    chain states {
            ip protocol tcp ct state established,related accept
            ip protocol udp ct state established,related accept
            ip protocol icmp ct state established,related accept
            return
    }
})SOPHOS";
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(RULES_FILE, expectedRulesContents,
                                                     "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(
            Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    EXPECT_CALL(*mockFilePermissions, getGroupId("sophos-spl-group")).WillOnce(Return(gid_t{1}));
    Tests::ScopedReplaceFilePermissions replaceFilePermissions{
            std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions)
    };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                InSequence s;

                std::vector<std::string> args = {"list", "table", "inet", "sophos_device_isolation"};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));

                args = {"-f", RULES_FILE};
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));


                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::applyIsolateRules(policy.exclusions());
    EXPECT_EQ(result, NftWrapper::IsolateResult::SUCCESS);
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
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
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

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::SUCCESS);
    EXPECT_TRUE(appenderContains("Successfully cleared Sophos network filtering rules"));
}

TEST_F(TestNftWrapper, clearIsolateRulesHandlesMissingBinary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(NFT_BINARY)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isExecutable(NFT_BINARY)).Times(0);
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::RULES_NOT_PRESENT);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::RULES_NOT_PRESENT);
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

    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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


    NftWrapper::IsolateResult result;
    EXPECT_NO_THROW(result = NftWrapper::clearIsolateRules());
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(1)
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED))
                        .RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
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
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, wait(_, _)).Times(2)
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED))
                        .RetiresOnSaturation();
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
    EXPECT_TRUE(appenderContains("The nft delete table command did not complete in time"));
}