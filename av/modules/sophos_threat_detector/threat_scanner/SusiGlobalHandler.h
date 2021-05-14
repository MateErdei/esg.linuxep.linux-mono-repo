/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <atomic>
#include <string>
#include <memory>
#include <mutex>

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
        bool update(const std::string& path);

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
        std::mutex m_initializeMutex;
    };
    using SusiGlobalHandlerSharePtr = std::shared_ptr<SusiGlobalHandler>;
}