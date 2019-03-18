/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

EVP_MD_CTX* EVP_MD_CTX_new(void);
void EVP_MD_CTX_free(EVP_MD_CTX* ctx);

static inline const unsigned char* ASN1_STRING_get0_data(ASN1_STRING* x)
{
    return ASN1_STRING_data(x);
}

#endif /* OPENSSL_VERSION_NUMBER */
