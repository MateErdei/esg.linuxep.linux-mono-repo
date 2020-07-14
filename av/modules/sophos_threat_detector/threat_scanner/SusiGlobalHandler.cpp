/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiGlobalHandler.h"

#include "Logger.h"

#include <Susi.h>

#include <iostream>
#include <cassert>

using namespace threat_scanner;

static bool isWhitelistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;

    return false;
}

static SusiCertTrustType isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)pkcs7;
    (void)size;

    return SUSI_TRUSTED;
}

static bool isWhitelistedCert(void *token, const char *fileTopLevelCert, size_t size)
{
    (void)token;
    (void)fileTopLevelCert;
    (void)size;

    return false;
}

static SusiCallbackTable my_susi_callbacks{
        .version = CALLBACK_TABLE_VERSION,
        .token = nullptr,
        .IsWhitelistedFile = isWhitelistedFile,
        .IsTrustedCert = isTrustedCert,
        .IsWhitelistedCert = isWhitelistedCert
};

void susiLogCallback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        LOGERROR(level << ": " << m);
    }
}

static const SusiLogCallback GL_log_callback{
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = susiLogCallback,
    .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

static void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}

SusiGlobalHandler::SusiGlobalHandler(const std::string& json_config)
{
    my_susi_callbacks.token = this;

    auto res = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(res, "Failed to set log callback");

    res = SUSI_Initialize(json_config.c_str(), &my_susi_callbacks);
    LOGINFO("Global Susi constructed res=" << std::hex << res << std::dec);
    assert(res == SUSI_S_OK);
}

SusiGlobalHandler::~SusiGlobalHandler()
{
    SusiResult res = SUSI_Terminate();
    LOGINFO("Global Susi destroyed res=" << std::hex << res << std::dec);
    assert(res == SUSI_S_OK);
}
