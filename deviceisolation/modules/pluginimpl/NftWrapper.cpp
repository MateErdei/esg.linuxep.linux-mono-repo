// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "NftWrapper.h"
#include "ApplicationPaths.h"
#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/Process/IProcess.h"

namespace Plugin
{
    IsolateResult NftWrapper::applyIsolateRules(const std::vector<Plugin::IsolationExclusion> &allowList)
    {
        auto fs = Common::FileSystem::fileSystem();

        if (!fs->exists(Plugin::nftBinary()))
        {
            LOGERROR("nft binary does not exist: " << Plugin::nftBinary());
            return IsolateResult::FAILED;
        }
        if (!fs->isExecutable(Plugin::nftBinary()))
        {
            LOGERROR("nft binary is not executable");
            return IsolateResult::FAILED;
        }
        auto rules = createIsolateRules(allowList);
        LOGDEBUG("Entire nftables ruleset: " << rules);

        const mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                            static_cast<int>(std::filesystem::perms::owner_write);
        const auto rulesFile = Plugin::networkRulesFile();

        fs->writeFileAtomically(rulesFile, rules, Plugin::pluginTempDir(), mode);

        auto process = ::Common::Process::createProcess();

        // Check table exists
        process->exec(Plugin::nftBinary(), {"list", "table", "inet", TABLE_NAME});
        auto status = process->wait(std::chrono::milliseconds(100), NFT_TIMEOUT_DS);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft list table command did not complete in time, killing process");
            process->kill();
            return IsolateResult::FAILED;
        }

        int exitCode = process->exitCode();
        // Checking for 0 as if table exists then isolation rules have already been enforced, no need to repeat
        if (exitCode == 0)
        {
            LOGINFO("nft list table output while applying rules: " << process->output());
            return IsolateResult::RULES_ALREADY_PRESENT;
        }

        //Read rules in from rulesFile
        process->exec(Plugin::nftBinary(), {"-f", rulesFile});
        status = process->wait(std::chrono::milliseconds(100), NFT_TIMEOUT_DS);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft command did not complete in time, killing process");
            process->kill();
            return IsolateResult::FAILED;
        }

        exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGERROR("Failed to set network rules, nft exit code: " << exitCode);
            LOGDEBUG("nft output for set network: " << process->output());
            return IsolateResult::FAILED;
        }

        LOGINFO("Successfully set network filtering rules");
        return IsolateResult::SUCCESS;
    }

    IsolateResult NftWrapper::clearIsolateRules()
    {
        //https://wiki.nftables.org/wiki-nftables/index.php/Configuring_tables
        // Linux kernels earlier than 3.18 require you to flush the table's contents first.
        // nft flush table inet TABLE
        // nft delete table inet TABLE

        auto fs = Common::FileSystem::fileSystem();
        if (!fs->exists(Plugin::nftBinary()))
        {
            LOGERROR("nft binary does not exist: " << Plugin::nftBinary());
            return IsolateResult::FAILED;
        }
        if (!fs->isExecutable(Plugin::nftBinary()))
        {
            LOGERROR("nft binary is not executable");
            return IsolateResult::FAILED;
        }

        auto process = ::Common::Process::createProcess();

        // Check table exists
        process->exec(Plugin::nftBinary(), {"list", "table", "inet", TABLE_NAME});
        auto status = process->wait(std::chrono::milliseconds(100), NFT_TIMEOUT_DS);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft list table command did not complete in time");
            process->kill();
            return IsolateResult::FAILED;
        }

        int exitCode = process->exitCode();
        if (exitCode == 1)
        {
            // The common case, when rules not present
            LOGDEBUG("nft exit code 1: rules probably not present");
            LOGDEBUG("nft output for list table: " << process->output());
            return IsolateResult::RULES_NOT_PRESENT;
        }
        else if (exitCode != 0)
        {
            LOGERROR("Failed to list table, nft exit code: " << exitCode);
            LOGERROR("nft output for list table: " << process->output());
            return IsolateResult::RULES_NOT_PRESENT;
        }

        // Flush table
        process->exec(Plugin::nftBinary(), {"flush", "table", "inet", TABLE_NAME});
        status = process->wait(std::chrono::milliseconds(100), NFT_TIMEOUT_DS);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft flush table command did not complete in time");
            process->kill();
            return IsolateResult::FAILED;
        }

        exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGERROR("Failed to flush table, nft exit code: " << exitCode);
            LOGDEBUG("nft output for flush table: " << process->output());
            return IsolateResult::FAILED;
        }

        // Delete table
        process->exec(Plugin::nftBinary(), {"delete", "table", "inet", TABLE_NAME});
        status = process->wait(std::chrono::milliseconds(100), NFT_TIMEOUT_DS);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft delete table command did not complete in time");
            process->kill();
            return IsolateResult::FAILED;
        }

        exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGERROR("Failed to delete table, nft exit code: " << exitCode);
            LOGDEBUG("nft output for delete table: " << process->output());
            return IsolateResult::FAILED;
        }

        LOGINFO("Successfully cleared Sophos network filtering rules");
        return IsolateResult::SUCCESS;
    }

    std::string NftWrapper::createIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList)
    {
        const auto* fp = Common::FileSystem::filePermissions();
        const auto groupId = fp->getGroupId("sophos-spl-group");
        return createIsolateRules(allowList, groupId);
    }

    std::string NftWrapper::createIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList, gid_t groupId)
    {
        std::stringstream rules;

        std::stringstream outgoingAllowListString;
        std::stringstream incomingAllowListString;

        LOGDEBUG("Processing " << allowList.size() << " allow-listed exclusions");
        constexpr const char* indent = "            ";
        for (const auto &exclusion: allowList)
        {
            // Generate both direction rules
            if (exclusion.direction() == Plugin::IsolationExclusion::BOTH)
            {
                // If there are no addresses specified, then we allow all specified ports to all addresses
                if (exclusion.remoteAddressesAndIpTypes().empty())
                {
                    for (const auto &localPort: exclusion.localPorts())
                    {
                        // Allow incoming to that port, and outgoing on that port
                        incomingAllowListString << indent << "tcp dport " << localPort << " accept\n"
                                                << indent << "udp dport " << localPort << " accept\n";

                        outgoingAllowListString << indent << "tcp sport " << localPort << " accept\n"
                                                << indent << "udp sport " << localPort << " accept\n";
                    }
                    for (const auto &remotePort: exclusion.remotePorts())
                    {
                        // Allow incoming from that port, and outgoing to that port
                        incomingAllowListString << indent << "tcp sport " << remotePort << " accept\n"
                                                << indent << "udp sport " << remotePort << " accept\n";

                        outgoingAllowListString << indent << "tcp dport " << remotePort << " accept\n"
                                                << indent << "udp dport " << remotePort << " accept\n";
                    }
                }
                else
                {
                    // If there are remote addresses specified, then we only allow ports to those remote addresses
                    for (const auto& [remoteAddress, ipType] : exclusion.remoteAddressesAndIpTypes())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            // Both + IP + no port
                            // Allow all traffic to and from that IP address
                            outgoingAllowListString << indent << ipType << " daddr " << remoteAddress << " accept\n";
                            incomingAllowListString << indent << ipType << " saddr " << remoteAddress << " accept\n";
                        }
                        else
                        {
                            // Both directions + IP + port
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                // Allow incoming to that port, and outgoing on that port

                                incomingAllowListString
                                    << indent << ipType << " saddr " << remoteAddress
                                    << " tcp dport " << localPort << " accept\n"
                                    << indent << ipType << " saddr " << remoteAddress
                                    << " udp dport " << localPort << " accept\n";

                                outgoingAllowListString
                                    << indent << ipType << " daddr " << remoteAddress
                                    << " tcp sport " << localPort << " accept\n"
                                    << indent << ipType << " daddr " << remoteAddress
                                    << " udp sport " << localPort << " accept\n";
                            }
                            for (const auto &remotePort: exclusion.remotePorts())
                            {

                                incomingAllowListString
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " tcp sport " << remotePort << " accept\n"
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " udp sport " << remotePort << " accept\n";

                                outgoingAllowListString
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " tcp dport " << remotePort << " accept\n"
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " udp dport " << remotePort << " accept\n";
                            }
                        }
                    }
                }
            }
            // Generate outbound rules
            else if (exclusion.direction() == Plugin::IsolationExclusion::OUT)
            {
                // If there are no addresses specified, then we allow all specified ports to all addresses
                if (exclusion.remoteAddressesAndIpTypes().empty())
                {
                    for (const auto &localPort: exclusion.localPorts())
                    {
                        // Allow incoming to that port, and outgoing on that port
                        incomingAllowListString
                            << indent << "tcp dport " << localPort
                            << " tcp flags != syn accept\n" // Ban new connections incoming
                            << indent << "udp dport " << localPort << " accept\n"; // Can't restrict incoming to established

                        outgoingAllowListString << indent << "tcp sport " << localPort << " accept\n"
                                                << indent << "udp sport " << localPort << " accept\n";
                    }
                    for (const auto &remotePort: exclusion.remotePorts())
                    {
                        // Allow incoming from that port, and outgoing to that port
                        incomingAllowListString
                                << indent << "tcp sport " << remotePort
                                << " tcp flags != syn accept\n" // Ban new connections incoming
                                << indent << "udp sport " << remotePort << " accept\n";

                        outgoingAllowListString << indent << "tcp dport " << remotePort << " accept\n"
                                                << indent << "udp dport " << remotePort << " accept\n";
                    }
                }
                else
                {
                    // OUT-only, With remote IPs:

                    // If there are remote addresses specified, then we only allow ports to those remote addresses
                    for (const auto& [remoteAddress, ipType] : exclusion.remoteAddressesAndIpTypes())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            outgoingAllowListString << indent << ipType << " daddr " << remoteAddress << " accept\n";

                            incomingAllowListString << indent << ipType << " saddr " << remoteAddress << " tcp flags != syn accept\n";
                            incomingAllowListString << indent << ipType << " saddr " << remoteAddress << " meta l4proto udp accept\n";
                        }
                        else
                        {
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                incomingAllowListString
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " tcp dport " << localPort << " tcp flags != syn accept\n"
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " udp dport " << localPort << " accept\n";

                                outgoingAllowListString
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " tcp sport " << localPort << " accept\n"
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " udp sport " << localPort << " accept\n";
                            }
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                incomingAllowListString
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " tcp sport " << remotePort << " tcp flags != syn accept\n"
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " udp sport " << remotePort << " accept\n";

                                outgoingAllowListString
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " tcp dport " << remotePort << " accept\n"
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " udp dport " << remotePort << " accept\n";
                            }
                        }
                    }
                }
            }
            // Generate inbound rules
            else if (exclusion.direction() == Plugin::IsolationExclusion::IN)
            {
                // If there are no addresses specified, then we allow all specified ports to all addresses
                if (exclusion.remoteAddressesAndIpTypes().empty())
                {
                    for (const auto &localPort: exclusion.localPorts())
                    {
                        // Allow incoming to that port, and outgoing on that port
                        incomingAllowListString
                            << indent << "tcp dport " << localPort << " accept\n"
                            << indent << "udp dport " << localPort << " accept\n";

                        outgoingAllowListString
                            << indent << "tcp sport " << localPort << " tcp flags != syn accept\n" // Ban new connections outbound
                            << indent << "udp sport " << localPort << " accept\n"; // Can't restrict UDP outbound
                    }
                    for (const auto &remotePort: exclusion.remotePorts())
                    {
                        // Allow incoming from that port, and outgoing to that port
                        incomingAllowListString
                            << indent << "tcp sport " << remotePort << " accept\n"
                            << indent << "udp sport " << remotePort << " accept\n";

                        outgoingAllowListString
                            << indent << "tcp dport " << remotePort << " tcp flags != syn accept\n"
                            << indent << "udp dport " << remotePort << " accept\n";
                    }
                }
                else
                {
                    // IN-only, With remote IPs:

                    // If there are remote addresses specified, then we only allow ports to those remote addresses
                    for (const auto& [remoteAddress, ipType] : exclusion.remoteAddressesAndIpTypes())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            incomingAllowListString << indent << ipType << " saddr " << remoteAddress << " accept\n";

                            outgoingAllowListString
                                << indent << ipType << " daddr " << remoteAddress << " tcp flags != syn accept\n"
                                << indent << ipType << " daddr " << remoteAddress << " meta l4proto udp accept\n";
                        }
                        else
                        {
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                incomingAllowListString
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " tcp dport " << localPort << " accept\n"
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " udp dport " << localPort << " accept\n";

                                outgoingAllowListString
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " tcp sport " << localPort << " tcp flags != syn accept\n"
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " udp sport " << localPort << " accept\n";
                            }
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                incomingAllowListString
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " tcp sport " << remotePort << " accept\n"
                                        << indent << ipType << " saddr " << remoteAddress
                                        << " udp sport " << remotePort << " accept\n";

                                outgoingAllowListString
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " tcp dport " << remotePort << " tcp flags != syn accept\n"
                                        << indent << ipType << " daddr " << remoteAddress
                                        << " udp dport " << remotePort << " accept\n";
                            }
                        }
                    }
                }
            }
        }

        // Allow Sophos Processes by group:
        outgoingAllowListString << indent << "meta skgid " << groupId << " accept";
        incomingAllowListString << indent << "meta skgid " << groupId << " accept";

        // Note - Some platforms automatically convert icmp to 1 and vice versa

        /*
         * Drop invalid packets by conntrack
            ct state invalid drop
         * Allow localhost loopback:
            iif "lo" accept
        * Allow dns responses
            tcp sport 53 tcp flags != syn accept
            udp sport 53 accept

        * Allow ICMP
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept

        * Allow DHCP
            ip6 version 6 udp dport 546 accept
            ip version 4 udp dport 68 accept
         */
        rules << "table inet " << TABLE_NAME << R"( {
    chain INPUT {
            type filter hook input priority filter; policy drop;
            ct state invalid drop
            iif "lo" accept
            tcp sport 53 tcp flags != syn accept
            udp sport 53 accept
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept
            ip6 version 6 udp dport 546 accept
            ip version 4 udp dport 68 accept
)" << incomingAllowListString.str()

   /*
    * Allow localhost loopback:
            oif "lo" accept

    * Allow established connections:
            ct state established,related accept

    * Allow DNS
            tcp dport 53 accept
            udp dport 53 accept

   * Allow ICMP
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept

   * Allow DHCP
            ip version 4 udp dport 67 accept
            ip6 version 6 udp dport 547 accept
    */

<< R"(
    }

    chain FORWARD {
            type filter hook forward priority filter; policy drop;
    }

    chain OUTPUT {
            type filter hook output priority filter; policy drop;
            oif "lo" accept
            tcp dport 53 accept
            udp dport 53 accept
            ip protocol icmp accept
            meta l4proto ipv6-icmp accept
            ip version 4 udp dport 67 accept
            ip6 version 6 udp dport 547 accept
)" << outgoingAllowListString.str() << R"(
            reject with tcp reset
    }
})";

        return rules.str();
    }
} // namespace Plugin