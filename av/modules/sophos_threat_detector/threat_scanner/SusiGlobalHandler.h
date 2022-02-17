/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/AutoFd.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace threat_scanner
{
    class SusiGlobalHandler
    {
    public:
        /**
         * Setup log4cplus logging.
         *
         */
        explicit SusiGlobalHandler();

        /**
         * Terminate SUSI if it has been initialized.
         * Restore basic logging.
         */
        ~SusiGlobalHandler();

        /**
         * Update SUSI if initialized, otherwise store the update for later.
         *
         * @param path
         * @return true if the update was successful or wasn't attempted.
         */
        bool update(const std::string& path, const std::string& lockfile);

        /**
         * Update SUSI if initialized, otherwise store the update for later.
         *
         * @param path
         * @return true if the update was successful or wasn't attempted.
         */
        bool reload(const std::string& config);

        /**
         * Initialize SUSI if it hasn't been initialized before
         * @param jsonConfig Configuration to initialize SUSI if required
         * @return true if we initialized susi
         */
        bool initializeSusi(const std::string& jsonConfig);

        /**
         * Report whether SUSI is initialized or not
         * @return true if SUSI is initialized
         */
        bool susiIsInitialized();

    private:
        std::atomic_bool m_susiInitialised = false;
        std::atomic_bool m_updatePending = false;
        std::string m_updatePath;
        std::string m_lockFile;
        std::mutex m_initializeMutex;
        bool m_susiVersionAlreadyLogged = false;

        /**
         * Update SUSI. Assumes that SUSI has been initialised
         *
         * @param path
         * @return true if update was successful
         */
        bool internal_update(const std::string& path, const std::string& lockfile);

        bool acquireLock(datatypes::AutoFd& fd);
        bool releaseLock(datatypes::AutoFd& fd);

        void logSusiVersion();
    };
    using SusiGlobalHandlerSharePtr = std::shared_ptr<SusiGlobalHandler>;
}