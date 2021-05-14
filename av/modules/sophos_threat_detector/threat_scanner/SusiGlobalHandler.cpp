/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiGlobalHandler.h"

#include "Logger.h"
#include "SusiCertificateFunctions.h"
#include "SusiLogger.h"
#include "ThrowIfNotOk.h"

#include <iostream>
#include <cassert>
#include <utility>

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

static const SusiLogCallback GL_log_callback{
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

bool SusiGlobalHandler::update(const std::string& path)
{
    if (!m_susiInitialised.load(std::memory_order_acquire))
    {
        m_updatePath = path;
        m_updatePending.store(true, std::memory_order_release);
        LOGDEBUG("Threat scanner update is pending");
        return true;
    }
    SusiResult res = SUSI_Update(path.c_str());
    if (res == SUSI_I_UPTODATE)
    {
        LOGDEBUG("Threat scanner is already up to date");
    }
    else if (res == SUSI_S_OK)
    {
        LOGINFO("Threat scanner successfully updated");
    }
    else
    {
        std::ostringstream ost;
        ost << "Failed to update SUSI: 0x" << std::hex << res << std::dec;
        LOGERROR(ost.str());
    }
    m_updatePending.store(false, std::memory_order_release);
    return res == SUSI_S_OK || res == SUSI_I_UPTODATE;
}

bool SusiGlobalHandler::initializeSusi(const std::string& jsonConfig)
{
    // First check atomic without lock
    if (m_susiInitialised.load(std::memory_order_acquire))
    {
        return false;
    }

    std::lock_guard initLock(m_initializeMutex);

    // Re-check in protected section
    if (m_susiInitialised.load(std::memory_order_acquire))
    {
        return false;
    }

    auto res = SUSI_Initialize(jsonConfig.c_str(), &my_susi_callbacks);
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
            update(m_updatePath);
        }
    }
    return true;
}

bool SusiGlobalHandler::susiIsInitialized()
{
    return m_susiInitialised;
}
