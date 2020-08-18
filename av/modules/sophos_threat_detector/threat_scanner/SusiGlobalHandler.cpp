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

using namespace threat_scanner;

static bool isWhitelistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;

    LOGWARN("isWhitelistedFile: size="<<size);

    return false;
}

static SusiCallbackTable my_susi_callbacks{
        .version = CALLBACK_TABLE_VERSION,
        .token = nullptr,
        .IsWhitelistedFile = isWhitelistedFile,
        .IsTrustedCert = threat_scanner::isTrustedCert,
        .IsWhitelistedCert = threat_scanner::isWhitelistedCert
};

static const SusiLogCallback GL_log_callback{
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = threat_scanner::susiLogCallback,
    .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

SusiGlobalHandler::SusiGlobalHandler(const std::string& json_config)
{
    my_susi_callbacks.token = this;

    auto res = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(res, "Failed to set log callback");

    res = SUSI_Initialize(json_config.c_str(), &my_susi_callbacks);
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
    }
}

SusiGlobalHandler::~SusiGlobalHandler()
{
    SusiResult res = SUSI_Terminate();
    LOGSUPPORT("Exiting Global Susi result =" << std::hex << res << std::dec);
    assert(res == SUSI_S_OK);
}
