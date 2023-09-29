/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "libcrypto-compat.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#    include <openssl/engine.h>

#    include <string.h>

static void* OPENSSL_zalloc(size_t num)
{
    void* ret = OPENSSL_malloc(num);

    if (ret != NULL)
        memset(ret, 0, num);
    return ret;
}

EVP_MD_CTX* EVP_MD_CTX_new(void)
{
    return reinterpret_cast<EVP_MD_CTX*>(OPENSSL_zalloc(sizeof(EVP_MD_CTX)));
}

void EVP_MD_CTX_free(EVP_MD_CTX* ctx)
{
    EVP_MD_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
}

#endif /* OPENSSL_VERSION_NUMBER */