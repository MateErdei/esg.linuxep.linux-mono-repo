/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessInfo.h>

#include <chrono>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        class ProcessProxy
        {
        public:
            explicit ProcessProxy(Common::Process::IProcessInfoPtr processInfo);
            ~ProcessProxy() noexcept;
            ProcessProxy(ProcessProxy&&) noexcept;

            /**
             * Move-assignment operator.
             * Need to provide our own implementation, since we need to swap m_running, not copy it.
             * @return
             */
            ProcessProxy& operator=(ProcessProxy&&) noexcept;

            // Don't allow copying
            ProcessProxy(const ProcessProxy&) = delete;
            ProcessProxy& operator=(const ProcessProxy&) = delete;

            /**
             * Stops the process if it is running.
             */
            void stop();
            std::chrono::seconds checkForExit();

            /**
             * If process is enabled, and is not running, and enough time has passed, start process.
             * If process is enabled, and is not running, and enough time has not passed, return amount of time to wait.
             * If process is disabled, and is running, stop process.
             *
             * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
             */
            std::chrono::seconds ensureStateMatchesOptions();


            void setEnabled(bool enabled);

        protected:
            /**
             * Information object for the process
             */
            Common::Process::IProcessInfoPtr m_processInfo;

        private:
            /**
             * Starts the process.
             */
            void start();

            /**
             * Get the current status of the process.
             * @return
             */
            Common::Process::ProcessStatus status();

            /**
             * Wait for the exit code from the process.
             * @return
             */
            int exitCode();

            void swap(ProcessProxy& other);

            /**
             * Execution object for the process
             */
            Common::Process::IProcessPtr m_process;
            /**
             * Full path to the executable
             */
            std::string m_exe;
            /**
             * True if the process is currently running.
             */
            bool m_running;

            /**
             * When the process process last died.
             */
            time_t m_deathTime;

            bool m_enabled;

        };
    } // namespace ProcessMonitoringImpl
} // namespace Common
