/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessInfo.h>
#include <Common/ProcessMonitoring/IProcessProxy.h>

#include <chrono>
#include <mutex>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        class ProcessProxy : public Common::ProcessMonitoring::IProcessProxy
        {
        public:
            explicit ProcessProxy(Common::Process::IProcessInfoPtr processInfo);
            ~ProcessProxy() noexcept override;
            ProcessProxy(ProcessProxy&&) noexcept;

            // Don't allow copying
            ProcessProxy(const ProcessProxy&) = delete;
            ProcessProxy& operator=(const ProcessProxy&) = delete;
            ProcessProxy(const ProcessProxy&&) = delete;

            /**
             * Stops the process if it is running.
             */
            // cppcheck-suppress virtualCallInConstructor
            void stop() override;
            /**
             * If not started or process finished don't check process for 1 hour
             * If stop requested and process still running check again in 5 seconds
             * @return number of seconds to wait before checking processes again.
             */
            std::pair<std::chrono::seconds, Process::ProcessStatus> checkForExit() override;

            /**
             * If process is enabled, and is not running, and enough time has passed, start process.
             * If process is enabled, and is not running, and enough time has not passed, return amount of time to wait.
             * If process is disabled, and is running, stop process.
             *
             * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
             */
            std::chrono::seconds ensureStateMatchesOptions() override;

            /**
             * Sets m_enabled which starts or stops the process on the next call to ensureStateMatchesOptions
             * @param enabled
             */
            void setEnabled(bool enabled) override;

            bool isRunning() override;

            std::string name() const override;

        protected:
            bool runningFlag();
            bool enabledFlag();

            /**
             * Information object for the process
             */
            Common::Process::IProcessInfoPtr m_processInfo;

        private:
            /**
             * Get the current status of the process.
             * @return
             */
            Common::Process::ProcessStatus status();

            void lock_acquired_stop();

            struct ProcessSharedState
            {
                ProcessSharedState();

                std::mutex m_mutex;
                bool m_enabled;

                /**
                 * True if the process is currently running.
                 */
                bool m_running;

                /**
                 * Execution object for the process
                 */
                Common::Process::IProcessPtr m_process;

                /**
                 * When the process process last died.
                 */
                time_t m_deathTime;

                time_t m_killIssuedTime;
            };

            /**
             * Starts the process.
             */
            void start();

            /**
             * Wait for the exit code from the process.
             * @return
             */
            int exitCode();

            void setCoreDumpMode(const bool mode);

            ProcessSharedState m_sharedState;
            /**
             * Full path to the executable
             */
            std::string m_exe;
        };
    } // namespace ProcessMonitoringImpl
} // namespace Common
