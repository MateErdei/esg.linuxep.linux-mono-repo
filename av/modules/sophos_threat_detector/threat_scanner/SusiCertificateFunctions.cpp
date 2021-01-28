/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiCertificateFunctions.h"

#include "Logger.h"


SusiCertTrustType threat_scanner::isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)pkcs7;
    (void)size;

    LOGDEBUG("Calling isTrustedCert with size=" << size);

    return SUSI_UNKNOWN;
}

bool threat_scanner::isAllowlistedCert(void *token, const char *fileTopLevelCert, size_t size)
{
    (void)token;
    (void)fileTopLevelCert;
    (void)size;

    LOGDEBUG("Calling isAllowlistedCert with size=" << size);

    return false;
}
