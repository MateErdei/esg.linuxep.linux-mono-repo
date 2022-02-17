/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiGlobalHandler.h"

#include "Logger.h"
#include "SusiCertificateFunctions.h"
#include "SusiLogger.h"
#include "ThrowIfNotOk.h"

#include <Common/Logging/LoggerConfig.h>

#include <cassert>
#include <iostream>

#include <sys/file.h>

using namespace threat_scanner;

static bool isAllowlistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;

    LOGDEBUG("isAllowlistedFile: size="<<size);

    return false;
}

static SusiCallbackTable my_susi_callbacks{ // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        .version = SUSI_CALLBACK_TABLE_VERSION,
        .token = nullptr,
        .IsAllowlistedFile = isAllowlistedFile,
        .IsTrustedCert = threat_scanner::isTrustedCert,
        .IsAllowlistedCert = threat_scanner::isAllowlistedCert
};

static SusiLogCallback GL_log_callback{ // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = threat_scanner::susiLogCallback,
    .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

static const SusiLogCallback GL_fallback_log_callback{
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = threat_scanner::fallbackSusiLogCallback,
    .minLogLevel = SUSI_LOG_LEVEL_INFO
};

SusiGlobalHandler::SusiGlobalHandler()
{
    my_susi_callbacks.token = this;
    auto log_level = std::min(getThreatScannerLogger().getLogLevel(), getSusiDebugLogger().getLogLevel());

    if (log_level >= Common::Logging::WARN)
    {
        GL_log_callback.minLogLevel = SUSI_LOG_LEVEL_WARNING;
    }
    else if (log_level >= Common::Logging::INFO)
    {
        GL_log_callback.minLogLevel = SUSI_LOG_LEVEL_INFO;
    }

    auto res = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(res, "Failed to set log callback");
}

SusiGlobalHandler::~SusiGlobalHandler()
{
    if (m_susiInitialised.load(std::memory_order_acquire))
    {
        auto res = SUSI_Terminate();
        LOGSUPPORT("Exiting Global Susi result =" << std::hex << res << std::dec);
        assert(res == SUSI_S_OK);
    }


    auto res = SUSI_SetLogCallback(&GL_fallback_log_callback);
    static_cast<void>(res); // Ignore res for non-debug builds (since we can't throw an exception in destructors)
    assert(res == SUSI_S_OK);
}

bool SusiGlobalHandler::reload(const std::string& config)
{
    if(!susiIsInitialized())
    {
        LOGDEBUG("Susi not initialized, skipping global configuration reload");
        return false;
    }

    SusiResult res = SUSI_UpdateGlobalConfiguration(config.c_str());
    if (res == SUSI_S_OK)
    {
        LOGDEBUG("Susi configuration reloaded");
        return true;
    }
    else
    {
        LOGDEBUG("Susi configuration reload failed");
        return false;
    }
}

bool SusiGlobalHandler::update(const std::string& path, const std::string& lockfile)
{
    /*
     * We have to hold the init lock while checking if we have init,
     * and saving the values if not.
     *
     * We don't need to hold the lock afterwards, since init must have completed, and won't be run again.
     *
     * We have to hold the lock, otherwise we could get interrupted on the "m_updatePath = path;" line, and
     * init could finish, causing no-one to complete the update.
     */
    {
        // We assume update is rare enough to just always take the lock
        std::lock_guard initLock(m_initializeMutex);

        if (!m_susiInitialised.load(std::memory_order_acquire))
        {
            m_updatePath = path;
            m_lockFile = lockfile;
            m_updatePending.store(true, std::memory_order_release);
            LOGDEBUG("Threat scanner update is pending");
            return true;
        }
    }
    return internal_update(path, lockfile);
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
    struct timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 500000000L;

    while(!acquireLock(fd))
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
    SusiResult res = SUSI_Update(path.c_str());
    if (res == SUSI_I_UPTODATE)
    {
        LOGDEBUG("Threat scanner is already up to date");
    }
    else if (res == SUSI_S_OK)
    {
        LOGINFO("Threat scanner successfully updated");
        m_susiVersionAlreadyLogged = true;
        logSusiVersion();
    }
    else
    {
        std::ostringstream ost;
        ost << "Failed to update SUSI: 0x" << std::hex << res << std::dec;
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
    return res == SUSI_S_OK || res == SUSI_I_UPTODATE;
}

bool SusiGlobalHandler::initializeSusi(const std::string& jsonConfig)
{
    // First check atomic without lock
    if (m_susiInitialised.load(std::memory_order_acquire))
    {
        LOGDEBUG("SUSI already initialised");
        return false;
    }

    std::lock_guard initLock(m_initializeMutex);

    // Re-check in protected section
    if (m_susiInitialised.load(std::memory_order_acquire))
    {
        LOGDEBUG("SUSI already initialised - inside lock");
        return false;
    }

    LOGINFO("Initializing SUSI");
    auto res = SUSI_Initialize(jsonConfig.c_str(), &my_susi_callbacks);
    // gets set to true in internal_update only when the update returns SUSI_OK
    m_susiVersionAlreadyLogged = false;
    if (res != SUSI_S_OK)
    {
        // This can fail for reasons outside the programs control, therefore is an exception
        // rather then an assert
        std::ostringstream ost;
        ost << "Failed to initialise SUSI: 0x" << std::hex << res << std::dec;
        LOGERROR(ost.str());
        throw std::runtime_error(ost.str());
    }
    else
    {
        LOGSUPPORT("Initialising Global Susi successful");
        m_susiInitialised.store(true, std::memory_order_release); // susi init is now saved
        if (m_updatePending.load(std::memory_order_acquire))
        {
            LOGDEBUG("Threat scanner triggering pending update");
            internal_update(m_updatePath, m_lockFile);
            LOGDEBUG("Threat scanner pending update completed");
        }
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

void SusiGlobalHandler::logSusiVersion()
{
    SusiVersionResult* result = nullptr;
    auto res = SUSI_GetVersion(&result);
    if(res == SUSI_S_OK)
    {
        LOGINFO("SUSI Libraries loaded: " << result->versionResultJson);
    }
    else
    {
        std::ostringstream ost;
        ost << "Failed to retrieve SUSI version: 0x" << std::hex << res << std::dec;
        LOGERROR(ost.str());
        throw std::runtime_error(ost.str());
    }
}