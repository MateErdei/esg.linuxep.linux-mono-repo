/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Susi.h>

namespace threat_scanner
{
    SusiCertTrustType isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size);

    bool isAllowlistedCert(void *token, const char *fileTopLevelCert, size_t size);

    bool isBlocklistedCert(void *token, const char *fileTopLevelCert, size_t size);
}
