// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISusiGlobalHandler.h"

#include "SusiApiWrapper.h"

#include "datatypes/AutoFd.h"

#include "common/ThreatDetector/SusiSettings.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace threat_scanner
{
    class SusiGlobalHandler final : public ISusiGlobalHandler
    {
    public:
        /**
         * Setup log4cplus logging.
         *
         */
        explicit SusiGlobalHandler(std::shared_ptr<ISusiApiWrapper> susiWrapper = std::make_shared<SusiApiWrapper>());

        /**
         * Terminate SUSI if it has been initialized.
         * Restore basic logging.
         */
        ~SusiGlobalHandler() override;

        /**
         * Update SUSI if initialized, otherwise store the update for later.
         *
         * @param path
         * @return true if the update was successful or wasn't attempted.
         */
        bool update(const std::string& path, const std::string& lockfile);

        /**
         * Reload SUSI global config if SUSI is initialized.
         *
         * @param path
         * @return true if the reload was successful or wasn't attempted.
         */
        bool reload(const std::string& config);

        /**
         * Set flag to cancel all remaining SUSI activities
         *
         */
        void shutdown();

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

        /**
         * Report whether we're shutting down
         * @return true if we've been told to shut down
         */
        bool isShuttingDown();

        /**
         * Bootstraps SUSI, will always attempt to bootstrap
         * make sure there is no distribution_version directory when calling it
         * @return
         */
        SusiResult bootstrap();

        /**
         * Return copy of std::shared_ptr<common::ThreatDetector::SusiSettings> so that when we swap in a new set of
         * settings anything left with the old instance is safe until it re-acquires a new copy.
         */
        std::shared_ptr<common::ThreatDetector::SusiSettings> accessSusiSettings();
        void setSusiSettings(std::shared_ptr<common::ThreatDetector::SusiSettings>&& settings);

        /**
         * @return True if machine learning detection should be enabled
         */
        bool isMachineLearningEnabled();

        /**
         * Check if SHA256 sum if allowed
         * @param threatChecksum
         * @return
         */
        bool isAllowListed(const std::string& threatChecksum) final;

        bool isPuaApproved(const std::string& puaName) final;

        bool isOaPuaDetectionEnabled() final;

    private:
        std::atomic_bool m_susiInitialised = false;
        std::atomic_bool m_updatePending = false;
        std::atomic_bool m_shuttingDown = false;
        std::string m_updatePath;
        std::string m_lockFile;
        std::mutex m_globalSusiMutex;
        bool m_susiVersionAlreadyLogged = false;
        bool m_machineLearningAlreadyLogged = false;

        // Used to make sure we don't change the settings while they're being used.
        std::mutex m_susiSettingsMutex;
        std::shared_ptr<common::ThreatDetector::SusiSettings> m_susiSettings;

        std::shared_ptr<ISusiApiWrapper> m_susiWrapper;

        /**
         * Update SUSI. Assumes that SUSI has been initialised
         * and caller already holds m_globalSusiMutex
         * @param path
         * @return true if update was successful
         */
        bool internal_update(const std::string& path, const std::string& lockfile);

        static bool acquireLock(datatypes::AutoFd& fd);
        static bool releaseLock(datatypes::AutoFd& fd);

        void logSusiVersion();
    };
    using SusiGlobalHandlerSharedPtr = std::shared_ptr<SusiGlobalHandler>;
}