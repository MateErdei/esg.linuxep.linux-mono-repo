/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiGlobalHandler.h"

#include "Logger.h"
#include "SusiCertificateFunctions.h"
#include "SusiLogger.h"
#include "ThrowIfNotOk.h"

#include "common/ShuttingDownException.h"

#include <Common/Logging/LoggerConfig.h>

#include <sys/file.h>
#include <sys/stat.h>

#include <cassert>
#include <filesystem>
#include <iostream>

namespace threat_scanner
{
    namespace
    {
        bool isAllowlistedFile(void* token, SusiHashAlg algorithm, const char* fileChecksum, size_t size)
        {
            (void)token;
            (void)algorithm;
            (void)fileChecksum;
            (void)size;

            LOGDEBUG("isAllowlistedFile: size=" << size);

            return false;
        }

        bool IsBlocklistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
        {
            (void)token;
            (void)algorithm;
            (void)fileChecksum;
            (void)size;

            LOGDEBUG("IsBlocklistedFile: size=" << size);

            return false;
        }

        SusiCallbackTable my_susi_callbacks{
            .version = SUSI_CALLBACK_TABLE_VERSION,
            .token = nullptr, //NOLINT
            .IsAllowlistedFile = isAllowlistedFile,
            .IsBlocklistedFile = IsBlocklistedFile,
            .IsTrustedCert = isTrustedCert,
            .IsAllowlistedCert = isAllowlistedCert
        };

        SusiLogCallback GL_log_callback {
            .version = SUSI_LOG_CALLBACK_VERSION,
            .token = nullptr,
            .log = threat_scanner::susiLogCallback,
            .minLogLevel = SUSI_LOG_LEVEL_DETAIL
        };

        const SusiLogCallback GL_fallback_log_callback {
            .version = SUSI_LOG_CALLBACK_VERSION,
            .token = nullptr,
            .log = threat_scanner::fallbackSusiLogCallback,
            .minLogLevel = SUSI_LOG_LEVEL_INFO
        };
    }

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
        std::filesystem::path updateSource = "/susi/update_source";
        std::filesystem::path installDest = "/susi/distribution_version";

        auto prevUmask = ::umask(023);

        LOGINFO("Bootstrapping SUSI from update source: " << updateSource);
        SusiResult susiResult =  m_susiWrapper->SUSI_Install(updateSource.c_str(), installDest.c_str());
        if (susiResult == SUSI_S_OK)
        {
            LOGINFO("Successfully installed SUSI to: " << installDest);
        }
        else
        {
            LOGERROR("Failed to bootstrap SUSI with error: " << susiResult);
        }

        ::umask(prevUmask);
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
            throw std::runtime_error(errorMsg.str());
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
                throw std::runtime_error(errorMsg.str());
            }
            nanosleep(&timeout, nullptr);
        }
        LOGDEBUG("Acquired lock on " << lockfile);

        assert(m_susiInitialised.load(std::memory_order_acquire));
        // SUSI is always initialised by the time we get here
        LOGDEBUG("Calling SUSI_Update");
        SusiResult updateResult =  m_susiWrapper->SUSI_Update(path.c_str());
        if (updateResult == SUSI_I_UPTODATE)
        {
            LOGDEBUG("Threat scanner is already up to date");
        }
        else if (!SUSI_FAILURE(updateResult))
        {
            LOGINFO("Threat scanner successfully updated");
            if (updateResult == SUSI_W_OLDDATA)
            {
                LOGWARN("SUSI Loaded old data");
            }
            m_susiVersionAlreadyLogged = true;
            logSusiVersion();
        }
        else
        {
            std::ostringstream ost;
            ost << "Failed to update SUSI: 0x" << std::hex << updateResult << std::dec;
            LOGERROR(ost.str());
        }

        m_updatePending.store(false, std::memory_order_release);
        if (releaseLock(fd))
        {
            LOGDEBUG("Released lock on " << lockfile);
        }
        else
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to release lock on " << lockfile;
            throw std::runtime_error(errorMsg.str());
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

        if(std::filesystem::is_directory("/susi/distribution_version"))
        {
            LOGINFO("SUSI already bootstrapped");
        }
        else
        {
            auto bootstrapResult = bootstrap();
            throwIfNotOk(bootstrapResult, "Bootstrapping SUSI failed, exiting");
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

            std::filesystem::remove("/susi/distribution_version");

            auto bootstrapResult = bootstrap();
            throwIfNotOk(bootstrapResult, "Bootstrapping SUSI at re-initialization failed, exiting");

            SusiResult initSecondAttempt =  m_susiWrapper->SUSI_Initialize(jsonConfig.c_str(), &my_susi_callbacks);
            throwIfNotOk(initSecondAttempt, "Second attempt to initialise SUSI failed, exiting");
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
            internal_update(m_updatePath, m_lockFile);
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
            throw std::runtime_error(ost.str());
        }

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
} // namespace threat_scanner