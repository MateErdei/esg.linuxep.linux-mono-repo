// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessInfo.h"
#include "Common/ProcessMonitoring/IProcessProxy.h"

#include <atomic>
#include <chrono>
#include <mutex>

namespace Common::ProcessMonitoringImpl
{
    class ProcessProxy : public Common::ProcessMonitoring::IProcessProxy
    {
    public:
        using exit_status_t = Common::ProcessMonitoring::IProcessProxy::exit_status_t;
        static constexpr int RESTART_EXIT_CODE = 77;
        explicit ProcessProxy(Common::Process::IProcessInfoPtr processInfo);
        ~ProcessProxy() noexcept override;
        ProcessProxy(ProcessProxy&&) noexcept = delete;

        // Don't allow copying
        ProcessProxy(const ProcessProxy&) = delete;
        ProcessProxy& operator=(const ProcessProxy&) = delete;
        ProcessProxy(const ProcessProxy&&) = delete;
        void shutDownProcessCheckForExit() override;

        /**
         * Stops the process if it is running.
         */
        // cppcheck-suppress virtualCallInConstructor
        void stop() final;
        /**
         * If not started or process finished don't check process for 1 hour
         * If stop requested and process still running check again in 5 seconds
         * @return number of seconds to wait before checking processes again.
         */
        exit_status_t checkForExit() override;

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

        [[nodiscard]] std::string name() const override;

        void setTerminationCallbackNotifyPipe(NotifyPipeSharedPtr pipe) override
        {
            m_processTerminationCallbackPipe = std::move(pipe);
        }

    protected:
        bool runningFlag();
        bool enabledFlag();

        /**
         * Information object for the process
         */
        Common::Process::IProcessInfoPtr m_processInfo;

        /**
         * Wait for the exit code from the process.
         * @return
         */
        int exitCode();

        /**
         * Get the native Exit code from the process - allows distinguishing
         * between exit code and signals
         * @return
         */
        int nativeExitCode();

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

            int m_lastExit = 0;
        };

        void notifyProcessTerminationCallbackPipe();

        /**
         * Pipe that we notify when the process is terminating, during the callback from BoostProcessHolder
         */
        NotifyPipeSharedPtr m_processTerminationCallbackPipe;
        /**
         * Boolean to indicate we have come through the callback.
         * So we should wait for a result.
         */
        std::atomic_bool m_termination_callback_called = false;

        /**
         * Starts the process.
         */
        void start();

        void setCoreDumpMode(bool mode) override;

        ProcessSharedState m_sharedState;
        /**
         * Full path to the executable
         */
        std::string m_exe;
    };
} // namespace Common::ProcessMonitoringImpl
