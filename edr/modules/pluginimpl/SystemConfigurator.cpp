// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "Logger.h"
#include "SystemConfigurator.h"

#include "EdrCommon/ApplicationPaths.h"

#include "Common/Process/IProcess.h"

#include <thread>

namespace Plugin
{
    void SystemConfigurator::setupOSForAudit(bool disableAuditD)
    {
        std::string auditdServiceName("auditd");
        if (disableAuditD)
        {
            LOGINFO("EDR configuration set to disable AuditD");
            if (checkIfServiceActive(auditdServiceName))
            {
                try
                {
                    stopAndDisableSystemService(auditdServiceName);
                }
                catch (std::exception& ex)
                {
                    LOGERROR("Failed to stop and disable auditd");
                }
            }
            else
            {
                LOGINFO("AuditD not found on system or is not active.");
            }
            breakLinkBetweenJournaldAndAuditSubsystem();
        }
        else
        {
            LOGINFO("EDR configuration set to not disable AuditD");
            if (checkIfServiceActive(auditdServiceName))
            {
                LOGWARN("AuditD is running, it will not be possible to obtain event data.");
            }
        }
    }

    bool SystemConfigurator::checkIfServiceActive(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { "is-active", serviceName });

        return (process->exitCode() == 0);
    }

    void SystemConfigurator::stopAndDisableSystemService(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { "stop", serviceName });

        if (process->exitCode() == 4)
        {
            // handle error: unit auditd.service may be requested by dependency only
            // try to stop the service again using service command
            process->exec(Plugin::servicePath(), { serviceName, "stop" });
        }

        if (process->exitCode() == 0)
        {
            LOGINFO("Successfully stopped service: " << serviceName);
            process->exec(Plugin::systemctlPath(), { "disable", serviceName });
            if (process->exitCode() == 0)
            {
                LOGINFO("Successfully disabled service: " << serviceName);
            }
            else
            {
                LOGWARN("Failed to disable service: " << serviceName);
            }
        }
        else
        {
            LOGWARN("Failed to stop service: " << serviceName);
        }
    }

    std::tuple<int, std::string> SystemConfigurator::runSystemCtlCommand(
        const std::string& command,
        const std::string& target)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { command, target });
        return std::make_tuple(process->exitCode(), process->output());
    }

    bool SystemConfigurator::checkIfJournaldLinkedToAuditSubsystem()
    {
        auto [errorCode, output] = runSystemCtlCommand("is-enabled", "systemd-journald-audit.socket");
        LOGSUPPORT("journal-audit.socket is enabled returned (code:" << errorCode << ") " << output);
        return output.find("masked") != 0;
    }

    void SystemConfigurator::maskJournald()
    {
        LOGINFO("Masking journald audit socket");
        auto [maskExitCode, maskOutput] = runSystemCtlCommand("mask", "systemd-journald-audit.socket");
        if (maskExitCode != 0)
        {
            std::stringstream ss;
            ss << "Masking systemd-journald-audit.socket failed, exit code: " << maskExitCode
               << ", output: " << maskOutput;
            throw std::runtime_error(ss.str());
        }

        int timesToTry = 5;
        int msToWait = 200;
        for (int i = 0; i < timesToTry; ++i)
        {
            if (!checkIfJournaldLinkedToAuditSubsystem())
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(msToWait));
        }

        std::stringstream ss;
        ss << "systemd-journald-audit.socket was not masked after " << timesToTry * msToWait << "ms";
        throw std::runtime_error(ss.str());
    }

    void SystemConfigurator::breakLinkBetweenJournaldAndAuditSubsystem()
    {
        if (checkIfJournaldLinkedToAuditSubsystem())
        {
            try
            {
                maskJournald();

                // Restart systemd-journald service
                auto [stopJournaldExitCode, stopJournaldOutput] = runSystemCtlCommand("stop", "systemd-journald");
                if (stopJournaldExitCode != 0)
                {
                    LOGERROR(
                        "Failed to stop systemd-journald, exit code: " << stopJournaldExitCode
                                                                       << ", output: " << stopJournaldOutput);
                }

                auto [startJournaldExitCode, startJournaldOutput] = runSystemCtlCommand("start", "systemd-journald");
                if (startJournaldExitCode != 0)
                {
                    LOGERROR(
                        "Failed to start systemd-journald, exit code: " << startJournaldExitCode
                                                                        << ", output: " << startJournaldOutput);
                }
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to mask journald audit socket: " << ex.what());
            }
        }
        else
        {
            LOGDEBUG("Journald audit socket is already masked");
        }
    }

} // namespace Plugin
