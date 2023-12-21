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
#include "deviceisolation/modules/pluginimpl/NTPPolicy.h"

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
                std::vector<std::string> args = {"-f", RULES_FILE};
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
                std::vector<std::string> args = {"-f", RULES_FILE};
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
                std::vector<std::string> args = {"-f", RULES_FILE};
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

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> args = {"-f", RULES_FILE};
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

                std::vector<std::string> args = { "list", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec(NFT_BINARY, args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                        .WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    NftWrapper::IsolateResult result = NftWrapper::clearIsolateRules();
    EXPECT_EQ(result, NftWrapper::IsolateResult::FAILED);
}
