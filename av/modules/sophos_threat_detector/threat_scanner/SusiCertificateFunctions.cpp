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

    LOGWARN("Calling isTrustedCert with size=" << size);

    return SUSI_TRUSTED;
}
