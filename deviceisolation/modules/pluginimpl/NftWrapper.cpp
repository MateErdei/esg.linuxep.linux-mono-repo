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
        const auto rulesFile = Plugin::networkRulesFile();
        std::stringstream rules;

        std::string outgoingAllowListString;
        std::string incomingAllowListString;

        LOGDEBUG("Processing " << allowList.size() << " allow-listed exclusions");
        const std::string indent = "            ";
        for (const auto &exclusion: allowList)
        {
            // Generate outbound rules
            if (exclusion.direction() == Plugin::IsolationExclusion::OUT ||
                exclusion.direction() == Plugin::IsolationExclusion::BOTH)
            {
                // If there are no addresses specified, then we allow all specified ports to all addresses
                if (exclusion.remoteAddressesAndIpTypes().empty())
                {
                    for (const auto &localPort: exclusion.localPorts())
                    {
                        outgoingAllowListString += indent + "tcp sport " + localPort + " accept\n";
                        outgoingAllowListString += indent + "udp sport " + localPort + " accept\n";
                    }
                    for (const auto &remotePort: exclusion.remotePorts())
                    {
                        outgoingAllowListString += indent + "tcp dport " + remotePort + " accept\n";
                        outgoingAllowListString += indent + "udp dport " + remotePort + " accept\n";
                    }
                }
                else
                {
                    // If there are remote addresses specified, then we only allow ports to those remote addresses
                    for (const auto& [remoteAddress, ipType] : exclusion.remoteAddressesAndIpTypes())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            outgoingAllowListString += indent + ipType + " daddr " + remoteAddress + " accept\n";
                        }
                        else
                        {
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                outgoingAllowListString +=
                                        indent + ipType + " daddr " + remoteAddress + " tcp dport " + remotePort + " accept\n";
                                outgoingAllowListString +=
                                        indent + ipType + " daddr " + remoteAddress + " udp dport " + remotePort + " accept\n";
                            }
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                outgoingAllowListString +=
                                        indent + ipType + " daddr " + remoteAddress + " tcp sport " + localPort + " accept\n";
                                outgoingAllowListString +=
                                        indent + ipType + " daddr " + remoteAddress + " udp sport " + localPort + " accept\n";
                            }
                        }
                    }
                }
            }

            // Generate inbound rules
            if (exclusion.direction() == Plugin::IsolationExclusion::IN ||
                exclusion.direction() == Plugin::IsolationExclusion::BOTH)
            {
                // If there are no addresses specified, then we allow all specified remote ports in
                if (exclusion.remoteAddressesAndIpTypes().empty())
                {
                    for (const auto &localPort: exclusion.localPorts())
                    {
                        incomingAllowListString +=
                                indent + "tcp dport " + localPort + " accept\n";
                        incomingAllowListString +=
                                indent + "udp dport " + localPort + " accept\n";
                    }
                    for (const auto &remotePort: exclusion.remotePorts())
                    {
                        incomingAllowListString +=
                                indent + "tcp sport " + remotePort + " accept\n";
                        incomingAllowListString +=
                                indent + "udp sport " + remotePort + " accept\n";
                    }
                }
                else
                {
                    // If there are remote addresses, then we only allow ports from those remote addresses
                    for (const auto& [remoteAddress, ipType] : exclusion.remoteAddressesAndIpTypes())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            incomingAllowListString += indent + ipType + " saddr " + remoteAddress + " accept\n";
                        }
                        else
                        {
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                incomingAllowListString +=
                                        indent + ipType + " saddr " + remoteAddress + " tcp sport " + remotePort +
                                        " accept\n";
                                incomingAllowListString +=
                                        indent + ipType + " saddr " + remoteAddress + " udp sport " + remotePort +
                                        " accept\n";
                            }
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                incomingAllowListString +=
                                        indent + ipType + " saddr " + remoteAddress + " tcp dport " + localPort +
                                        " accept\n";
                                incomingAllowListString +=
                                        indent + ipType + " saddr " + remoteAddress + " udp dport " + localPort +
                                        " accept\n";
                            }
                        }
                    }
                }
            }
        }

        auto fp = Common::FileSystem::filePermissions();
        auto groupId = fp->getGroupId("sophos-spl-group");

        outgoingAllowListString += indent + "meta skgid " + std::to_string(groupId) + " accept\n";

        // Note - Some platforms automatically convert icmp to 1 and vice versa

        rules << "table inet " << TABLE_NAME << R"( {
    chain INPUT {
            type filter hook input priority filter; policy drop;
            ct state invalid drop
            iifname "lo" accept
            jump states
            ip protocol icmp accept
)" << incomingAllowListString << R"(
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
)" << outgoingAllowListString << R"(
    }

    chain states {
            ip protocol tcp ct state established,related accept
            ip protocol udp ct state established,related accept
            ip protocol icmp ct state established,related accept
            return
    }
})";

        LOGDEBUG("Entire nftables ruleset: " << rules.str());
        mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                      static_cast<int>(std::filesystem::perms::owner_write);
        fs->writeFileAtomically(rulesFile, rules.str(), Plugin::pluginTempDir(), mode);

        auto process = ::Common::Process::createProcess();

        // Check table exists
        process->exec(Plugin::nftBinary(), {"list", "table", "inet", TABLE_NAME});
        auto status = process->wait(std::chrono::milliseconds(100), 500);
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
            return IsolateResult::RULES_NOT_PRESENT;
        }

        //Read rules in from rulesFile
        process->exec(Plugin::nftBinary(), {"-f", rulesFile});
        status = process->wait(std::chrono::milliseconds(100), 500);
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
        auto status = process->wait(std::chrono::milliseconds(100), 500);
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
        status = process->wait(std::chrono::milliseconds(100), 500);
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
        status = process->wait(std::chrono::milliseconds(100), 500);
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
} // namespace Plugin