// Copyright 2020-2024 Sophos Limited. All rights reserved.

#include "SusiGlobalHandler.h"

#include "Logger.h"
#include "ThrowIfNotOk.h"
#include "ThreatScannerException.h"

#include "common/ApplicationPaths.h"
#include "common/ShuttingDownException.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/LoggerConfig.h"

#include <nlohmann/json.hpp>

#include <sys/file.h>
#include <sys/stat.h>

#include <cassert>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace
{
    std::filesystem::path getChrootDir()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        try
        {
            return appConfig.getData("CHROOT");
        }
        catch (const std::exception& ex)
        {
            return "/";
        }
    }

}

namespace threat_scanner
{
    SusiGlobalHandler::SusiGlobalHandler(std::shared_ptr<ISusiApiWrapper> susiWrapper) :
        m_susiWrapper(std::move(susiWrapper))
    {
        my_susi_callbacks.token = this;
        auto log_level = std::min(getThreatScannerLogger().getChainedLogLevel(), getSusiDebugLogger().getChainedLogLevel());
        if (log_level >= Common::Logging::WARN)
        {
            GL_log_callback.minLogLevel = SUSI_LOG_LEVEL_WARNING;
        }
        else if (log_level >= Common::Logging::INFO)
        {
            GL_log_callback.minLogLevel = SUSI_LOG_LEVEL_INFO;
        }

        std::lock_guard susiLock(m_globalSusiMutex);

        auto res = m_susiWrapper->SUSI_SetLogCallback(&GL_log_callback);
        throwIfNotOk(res, "Failed to set log callback");
    }

    SusiGlobalHandler::~SusiGlobalHandler()
    {
        std::lock_guard susiLock(m_globalSusiMutex);

        if (m_susiInitialised.load(std::memory_order_acquire))
        {
            LOGDEBUG("Exiting Global Susi");
            auto res =  m_susiWrapper->SUSI_Terminate();
            LOGSUPPORT("Exiting Global Susi result = " << std::hex << res << std::dec);
            assert(!SUSI_FAILURE(res));
        }

        auto res =  m_susiWrapper->SUSI_SetLogCallback(&GL_fallback_log_callback);
        static_cast<void>(res); // Ignore res for non-debug builds (since we can't throw an exception in destructors)
        assert(!SUSI_FAILURE(res));
    }

    bool SusiGlobalHandler::reload(const std::string& config)
    {
        std::lock_guard susiLock(m_globalSusiMutex);

        if (!susiIsInitialized())
        {
            LOGDEBUG("Susi not initialized, skipping global configuration reload");
            return true;
        }

        LOGDEBUG("Reloading SUSI global configuration");
        SusiResult res = m_susiWrapper->SUSI_UpdateGlobalConfiguration(config.c_str());
        if (SUSI_FAILURE(res))
        {
            LOGWARN("Susi configuration reload failed");
            return false;
        }
        LOGDEBUG("Susi configuration reloaded");
        if (res == SUSI_W_OLDDATA)
        {
            LOGWARN("SUSI Loaded old data");
        }
        return true;
    }

    void SusiGlobalHandler::shutdown()
    {
        LOGDEBUG("SusiGlobalHandler got shutdown notification");
        m_shuttingDown.store(true, std::memory_order_release);
    }

    SusiResult SusiGlobalHandler::bootstrap()
    {
        std::filesystem::path updateSource = getChrootDir() / "susi/update_source";
        std::filesystem::path installDest = getChrootDir() / "susi/distribution_version";

        LOGINFO("Bootstrapping SUSI from update source: " << updateSource);
        SusiResult susiResult =  m_susiWrapper->SUSI_Install(updateSource.c_str(), installDest.c_str());
        if (!SUSI_FAILURE(susiResult))
        {
            LOGINFO("Successfully installed SUSI to: " << installDest);
            if (susiResult == SUSI_W_OLDDATA)
            {
                LOGWARN("SUSI Loaded old data");
            }
        }
        else
        {
            LOGERROR("Failed to bootstrap SUSI with error: " << susiResult);
        }

        return susiResult;
    }

    bool SusiGlobalHandler::update(const std::string& path, const std::string& lockfile)
    {
        /*
         * We have to hold the init lock while checking if we have init,
         * and saving the values if not.
         *
         * We have to hold the lock, otherwise we could get interrupted on the "m_updatePath = path;" line, and
         * init could finish, causing no-one to complete the update.
         */
        // We assume update is rare enough to just always take the lock
        std::lock_guard susiLock(m_globalSusiMutex);

        if (!m_susiInitialised.load(std::memory_order_acquire))
        {
            m_updatePath = path;
            m_lockFile = lockfile;
            m_updatePending.store(true, std::memory_order_release);
            LOGDEBUG("Threat scanner update is pending");
            return true;
        }

        return internal_update(path, lockfile);
    }

    bool SusiGlobalHandler::internal_update(const std::string& path, const std::string& lockfile)
    {
        mode_t mode = S_IRUSR | S_IWUSR;
        datatypes::AutoFd fd(open(lockfile.c_str(), O_RDWR | O_CREAT, mode));
        if (!fd.valid())
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to open lock file: " << lockfile;
            throw ThreatScannerException(LOCATION, errorMsg.str());
        }

        // Try up to 20 times 0.5s apart to acquire the file lock
        int maxRetries = 20;
        int attempt = 1;
        struct timespec timeout
        {
            .tv_sec = 0, .tv_nsec = 500000000L
        };

        while (!acquireLock(fd))
        {
            if (attempt++ >= maxRetries)
            {
                std::stringstream errorMsg;
                errorMsg << "Failed to acquire lock on " << lockfile;
                LOGERROR(errorMsg.str());
                throw ThreatScannerException(LOCATION, errorMsg.str());
            }
            nanosleep(&timeout, nullptr);
        }
        LOGDEBUG("Acquired lock on " << lockfile);

        assert(m_susiInitialised.load(std::memory_order_acquire));
        // SUSI is always initialised by the time we get here
        LOGDEBUG("Calling SUSI_Update");
        SusiResult updateResult =  m_susiWrapper->SUSI_Update(path.c_str());
        recordUpdateResult(updateResult);
        m_updatePending.store(false, std::memory_order_release);
        if (releaseLock(fd))
        {
            LOGDEBUG("Released lock on " << lockfile);
        }
        else
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to release lock on " << lockfile;
            throw ThreatScannerException(LOCATION, errorMsg.str());
        }
        return !SUSI_FAILURE(updateResult);
    }

    bool SusiGlobalHandler::initializeSusi(const std::string& jsonConfig)
    {
        // First check atomic without lock
        if (m_susiInitialised.load(std::memory_order_acquire))
        {
            LOGDEBUG("SUSI already initialised");
            return false;
        }

        std::lock_guard susiLock(m_globalSusiMutex);

        // Re-check in protected section
        if (m_susiInitialised.load(std::memory_order_acquire))
        {
            LOGDEBUG("SUSI already initialised - inside lock");
            return false;
        }

        if(std::filesystem::is_directory(getChrootDir() / "susi/distribution_version"))
        {
            LOGINFO("SUSI already bootstrapped");
        }
        else
        {
            auto bootstrapResult = bootstrap();
            throwFailedToInitIfNotOk(bootstrapResult, "Bootstrapping SUSI failed, exiting");
        }

        LOGINFO("Initializing SUSI");
        SusiResult initResult =  m_susiWrapper->SUSI_Initialize(jsonConfig.c_str(), &my_susi_callbacks); // Duplicate any changes here to 2nd init below
        // gets set to true in internal_update only when the update returns SUSI_OK
        m_susiVersionAlreadyLogged = false;

        if (SUSI_FAILURE(initResult))
        {
            std::ostringstream ost;
            ost << "Failed to initialise SUSI: 0x" << std::hex << initResult << std::dec;
            LOGERROR(ost.str());
            LOGINFO("Attempting to re-install SUSI");

            std::filesystem::remove_all(getChrootDir() / "susi/distribution_version");

            auto bootstrapResult = bootstrap();
            throwFailedToInitIfNotOk(bootstrapResult, "Bootstrapping SUSI at re-initialization failed, exiting");

            SusiResult initSecondAttempt =  m_susiWrapper->SUSI_Initialize(jsonConfig.c_str(), &my_susi_callbacks);
            throwFailedToInitIfNotOk(initSecondAttempt, "Second attempt to initialise SUSI failed, exiting");
        }

        LOGSUPPORT("Initialising Global Susi successful");
        m_susiInitialised.store(true, std::memory_order_release); // susi init is now saved

        if (isShuttingDown())
        {
            throw ShuttingDownException();
        }

        if (m_updatePending.load(std::memory_order_acquire))
        {
            LOGDEBUG("Threat scanner triggering pending update");
            bool success = internal_update(m_updatePath, m_lockFile);
            if (!success)
            {
                LOGERROR("Failed to update Threat scanner after initialisation - continuing to use existing data");
            }
            LOGDEBUG("Threat scanner pending update completed");
        }

        if (isShuttingDown())
        {
            throw ShuttingDownException();
        }

        if (!m_susiVersionAlreadyLogged)
        {
            logSusiVersion();
        }

        return true;
    }

    bool SusiGlobalHandler::susiIsInitialized()
    {
        return m_susiInitialised.load(std::memory_order_acquire);
    }

    bool SusiGlobalHandler::isShuttingDown()
    {
        return m_shuttingDown.load(std::memory_order_acquire);
    }

    void SusiGlobalHandler::logSusiVersion()
    {
        SusiVersionResult* result = nullptr;
        auto res =  m_susiWrapper->SUSI_GetVersion(&result);
        if (SUSI_FAILURE(res))
        {
            std::ostringstream ost;
            ost << "Failed to retrieve SUSI version: 0x" << std::hex << res << std::dec;
            LOGERROR(ost.str());
            throw ThreatScannerException(LOCATION, ost.str());
        }

        assert(result != nullptr);
        assert(result->versionResultJson != nullptr);
        LOGINFO("SUSI Libraries loaded: " << result->versionResultJson);
        if (res == SUSI_W_OLDDATA)
        {
            LOGWARN("SUSI Loaded old data");
        }
        m_susiWrapper->SUSI_FreeVersionResult(result);
    }

    bool SusiGlobalHandler::acquireLock(datatypes::AutoFd& fd)
    {
        if (flock(fd.get(), LOCK_EX | LOCK_NB) != 0)
        {
            return false;
        }
        return true;
    }

    bool SusiGlobalHandler::releaseLock(datatypes::AutoFd& fd)
    {
        if (flock(fd.get(), LOCK_UN | LOCK_NB) != 0)
        {
            return false;
        }
        return true;
    }

    void SusiGlobalHandler::loadSusiSettingsIfRequired()
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);
    }

    void SusiGlobalHandler::loadSusiSettingsIfRequiredLocked(std::lock_guard<std::mutex>&)
    {
        if (!m_susiSettings)
        {
            m_susiSettings =
                std::make_shared<common::ThreatDetector::SusiSettings>(Plugin::getSusiStartupSettingsPath());
        }
    }

    std::shared_ptr<common::ThreatDetector::SusiSettings> SusiGlobalHandler::accessSusiSettings()
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);
        return m_susiSettings;
    }

    void SusiGlobalHandler::setSusiSettings(std::shared_ptr<common::ThreatDetector::SusiSettings>&& settings)
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        m_susiSettings = std::move(settings);
    }

    bool SusiGlobalHandler::isMachineLearningEnabled()
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);

        // Check for override file
        auto* filesystem = Common::FileSystem::fileSystem();
        // /opt/sophos-spl/plugins/av/chroot/etc/sophos_susi_force_machine_learning
        if (filesystem->isFile(getChrootDir() / "etc/sophos_susi_force_machine_learning"))
        {
            if (!m_machineLearningAlreadyLogged)
            {
                LOGINFO("Overriding Machine Learning - enabling due to presence of sophos_susi_force_machine_learning");
                m_machineLearningAlreadyLogged = true;
            }
            return true;
        }

        bool setting = m_susiSettings->isMachineLearningEnabled();
        if (!m_machineLearningAlreadyLogged)
        {
            LOGINFO("Machine Learning " << (setting ? "enabled" : "disabled") << " from policy");
            m_machineLearningAlreadyLogged = true;
        }
        return setting;
    }

    bool SusiGlobalHandler::isAllowListedSha256(const std::string& threatChecksum)
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);
        return m_susiSettings->isAllowListedSha256(threatChecksum);
    }


    bool SusiGlobalHandler::isAllowListedPath(const std::string& threatPath)
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);
        return m_susiSettings->isAllowListedPath(threatPath);
    }

    bool SusiGlobalHandler::isPuaApproved(const std::string& puaName)
    {
        std::lock_guard<std::mutex> lock(m_susiSettingsMutex);
        loadSusiSettingsIfRequiredLocked(lock);
        return m_susiSettings->isPuaApproved(puaName);
    }

    /*
         * Called by SUSI when a threat is detected. Does not get called on eicars.
         * @param token - user data pointer which is currently set to be the current SusiGlobalHandler instance.
         * @param algorithm - the hashing type used for the checksum, we currently only support SHA256.
         * @param fileChecksum - bytes (unsigned char) that need to be converted to a hex string
         *  for example, if first byte in fileChecksum is 142 then that is converted to the two characters "8e".
         * @param size - size of filechecksum
         * @return bool - returns true if the file checksum is on the allow list.
     */
    bool SusiGlobalHandler::isAllowlistedFile(void* token, SusiHashAlg algorithm, const char* fileChecksum, size_t size)
    {
        if (algorithm == SUSI_SHA256_ALG)
        {
            std::vector<unsigned char> checksumBytes2(fileChecksum, fileChecksum + size);
            std::ostringstream stream;
            stream << std::hex << std::setfill('0') << std::nouppercase;
            std::for_each(
                checksumBytes2.cbegin(),
                checksumBytes2.cend(),
                [&stream](const auto& byte) { stream << std::setw(2) << int(byte); });

            auto susiHandler = static_cast<SusiGlobalHandler*>(token);
            if (susiHandler->accessSusiSettings()->isAllowListedSha256(stream.str()))
            {
                LOGDEBUG("Allowed by SHA256: " << stream.str());
                return true;
            }
            else
            {
                LOGTRACE("Denied allow list by SHA256 for: " << stream.str()); // Will be hit frequently
            }
        }
        else
        {
            LOGWARN("isAllowlistFile called with unsupported algorithm: " << algorithm);
        }

        return false;
    }

    /*
         * Called by SUSI when a threat is detected. Does not get called on eicars.
         * @param token - user data pointer which is currently set to be the current SusiGlobalHandler instance.
         * @param filePath - char* representing the file path
         * @return bool - returns true if the file checksum is on the allow list.
     */
    bool SusiGlobalHandler::IsAllowlistedPath(void* token, const char* filePath)
    {
        if (filePath == nullptr)
        {
            LOGERROR("Allow list by path not possible, filePath provided by SUSI is invalid");
            return false;
        }

        auto susiHandler = static_cast<SusiGlobalHandler*>(token);
        if (susiHandler->accessSusiSettings()->isAllowListedPath(filePath))
        {
            LOGDEBUG("Allowed by path: " << filePath);
            return true;
        }
        else
        {
            LOGTRACE("Denied allow list by path for: " << filePath); // Will be hit frequently
        }

        return false;
    }

    bool SusiGlobalHandler::IsBlocklistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
    {
        (void)token;
        (void)algorithm;
        (void)fileChecksum;
        (void)size;

        return false;
    }

    void SusiGlobalHandler::recordUpdateResult(SusiResult updateResult)
    {
        bool failure = SUSI_FAILURE(updateResult);

        nlohmann::json record;
        auto* filesystem = Common::FileSystem::fileSystem();

        record["result"] = updateResult;
        record["success"] = !failure;

        if (updateResult == SUSI_I_UPTODATE)
        {
            LOGDEBUG("Threat scanner is already up to date");
        }
        else if (!failure)
        {
            LOGINFO("Threat scanner successfully updated");
            if (updateResult == SUSI_W_OLDDATA)
            {
                LOGWARN("SUSI Loaded old data");
                record["message"] = "SUSI Loaded old data";
            }
            m_susiVersionAlreadyLogged = true;
            logSusiVersion();
        }
        else if (updateResult == SUSI_E_CORRUPTDATA)
        {
            LOGERROR("Failed to update SUSI: Corrupt data in update_source: 0x" << std::hex << updateResult << std::dec);
            record["message"] = "Corrupt data in update_source";
        }
        else
        {
            std::ostringstream ost;
            ost << "Failed to update SUSI: 0x" << std::hex << updateResult << std::dec;
            LOGERROR(ost.str());
            record["message"] = ost.str();
        }

        auto contents = record.dump();
        filesystem->writeFileAtomically(getChrootDir() / "var/update_status.json", contents, getChrootDir() / "tmp", 0640);
    }

} // namespace threat_scanner