// Copyright 2023 Sophos Limited. All rights reserved.

#include "NftWrapper.h"
#include "Common/FileSystem/IFileSystem.h"
#include "ApplicationPaths.h"
#include "Logger.h"
#include "Common/Process/IProcess.h"

namespace Plugin
{
    NftWrapper::IsolateResult
    NftWrapper::applyIsolateRules(const std::vector<Plugin::IsolationExclusion> &allowList)
    {

        // TODO LINUXDAR-7962 get all the needed GIDs.
        // add multiple lines like "meta skgid REPLACE_WITH_GID! accept" to the OUTPUT chain.

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
                if (exclusion.remoteAddresses().empty())
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
                    for (const auto &remoteAddress: exclusion.remoteAddresses())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            outgoingAllowListString += indent + "ip daddr " + remoteAddress + " accept\n";
                        }
                        else
                        {
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                outgoingAllowListString +=
                                        indent + "ip daddr " + remoteAddress + " tcp dport " + remotePort + " accept\n";
                                outgoingAllowListString +=
                                        indent + "ip daddr " + remoteAddress + " udp dport " + remotePort + " accept\n";
                            }
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                outgoingAllowListString +=
                                        indent + "ip daddr " + remoteAddress + " tcp sport " + localPort + " accept\n";
                                outgoingAllowListString +=
                                        indent + "ip daddr " + remoteAddress + " udp sport " + localPort + " accept\n";
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
                if (exclusion.remoteAddresses().empty())
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
                    for (const auto &remoteAddress: exclusion.remoteAddresses())
                    {
                        if (exclusion.remotePorts().empty() && exclusion.localPorts().empty())
                        {
                            incomingAllowListString += indent + "ip saddr " + remoteAddress + " accept\n";
                        }
                        else
                        {
                            for (const auto &remotePort: exclusion.remotePorts())
                            {
                                incomingAllowListString +=
                                        indent + "ip saddr " + remoteAddress + " tcp sport " + remotePort +
                                        " accept\n";
                                incomingAllowListString +=
                                        indent + "ip saddr " + remoteAddress + " udp sport " + remotePort +
                                        " accept\n";
                            }
                            for (const auto &localPort: exclusion.localPorts())
                            {
                                incomingAllowListString +=
                                        indent + "ip saddr " + remoteAddress + " tcp dport " + localPort +
                                        " accept\n";
                                incomingAllowListString +=
                                        indent + "ip saddr " + remoteAddress + " udp dport " + localPort +
                                        " accept\n";
                            }
                        }
                    }
                }
            }
        }

        // TODO LINUXDAR-7964 Apply sophos GID exclusion.

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
        process->exec(Plugin::nftBinary(), {"-f", rulesFile});
        auto status = process->wait(std::chrono::milliseconds(100), 500);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft command did not complete in time, killing process");
            process->kill();
            return IsolateResult::FAILED;
        }

        int exitCode = process->exitCode();

        if (exitCode != 0)
        {
            LOGERROR("Failed to set network rules, nft exit code: " << exitCode);
            LOGDEBUG("nft output: " << process->output());
            return IsolateResult::FAILED;
        }

        LOGINFO("Successfully set network filtering rules");
        return IsolateResult::SUCCESS;
    }

    NftWrapper::IsolateResult NftWrapper::clearIsolateRules()
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
        if (exitCode != 0)
        {
            LOGERROR("Failed to list table, nft exit code: " << exitCode);
            LOGDEBUG("nft output: " << process->output());
            return IsolateResult::WARN;
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
            LOGDEBUG("nft output: " << process->output());
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
            LOGDEBUG("nft output: " << process->output());
            return IsolateResult::FAILED;
        }

        LOGINFO("Successfully cleared Sophos network filtering rules");
        return IsolateResult::SUCCESS;
    }
} // namespace Plugin