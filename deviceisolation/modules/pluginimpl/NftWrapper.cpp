// Copyright 2023 Sophos Limited. All rights reserved.

#include "NftWrapper.h"
#include "base/modules/Common/FileSystem/IFileSystem.h"
#include "deviceisolation/modules/pluginimpl/ApplicationPaths.h"
#include "Logger.h"
#include "Common/Process/IProcess.h"

namespace Plugin
{
    // TODO LINUXDAR-7964 Remove [[maybe_unused]]
    NftWrapper::IsolateResult
    NftWrapper::applyIsolateRules([[maybe_unused]] const std::vector<Plugin::IsolationExclusion> &allowList)
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

        // TODO LINUXDAR-7964 Apply exclusions.

        LOGDEBUG("Incoming allow list: " << incomingAllowListString);
        LOGDEBUG("Outgoing allow list: " << outgoingAllowListString);

        // TODO LINUXDAR-7964 Apply sophos GID exclusion.

        rules << "table inet " << TABLE_NAME << R"( {
    chain INPUT {
            type filter hook input priority filter; policy drop;
            ct state invalid counter packets 0 bytes 0 drop
            iifname "lo" counter packets 0 bytes 0 accept
            counter packets 0 bytes 0 jump states
            ip protocol icmp counter packets 0 bytes 0 accept
    }

    chain FORWARD {
            type filter hook forward priority filter; policy drop;
    }

    chain OUTPUT {
            type filter hook output priority filter; policy drop;
            counter packets 0 bytes 0 jump outgoing-services
            oifname "lo" counter packets 0 bytes 0 accept
            counter packets 0 bytes 0 jump states
            counter packets 0 bytes 0 jump outgoing-services
    }

    chain outgoing-services {
            tcp dport 53 counter packets 0 bytes 0 accept
            udp dport 53 counter packets 0 bytes 0 accept
)" << outgoingAllowListString << R"(
            ip protocol icmp counter packets 0 bytes 0 accept
    }

    chain states {
            ip protocol tcp ct state established,related counter packets 0 bytes 0 accept
            ip protocol udp ct state established,related counter packets 0 bytes 0 accept
            ip protocol icmp ct state established,related counter packets 0 bytes 0 accept
            counter packets 0 bytes 0 return
    }
})";

        LOGDEBUG("Entire nftables ruleset: " << rules.str());
        mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                      static_cast<int>(std::filesystem::perms::owner_write);
        fs->writeFileAtomically(rulesFile, rules.str(), Plugin::pluginTempDir(), mode);

        auto process = ::Common::Process::createProcess();




        // TODO LINUXDAR-7964 Remove -c option once we have allow listing working.
        process->exec(Plugin::nftBinary(), {"-c", "-f", rulesFile});
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

        // Flush table
        process->exec(Plugin::nftBinary(), {"flush", "table", "inet", TABLE_NAME});
        auto status = process->wait(std::chrono::milliseconds(100), 500);
        if (status != Common::Process::ProcessStatus::FINISHED)
        {
            LOGERROR("The nft flush table command did not complete in time");
            process->kill();
            return IsolateResult::FAILED;
        }

        int exitCode = process->exitCode();
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