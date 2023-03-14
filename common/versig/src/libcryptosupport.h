// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "hash_algorithm.h"
#include "hash_exception.h"
#include "libcrypto-compat.h"

namespace crypto
{
    inline const EVP_MD* construct_digest_algorithm(crypto::hash_algo algo)
    {
        switch (algo)
        {
            case crypto::hash_algo::ALGO_MD5:
                return EVP_md5();
            case crypto::hash_algo::ALGO_SHA1:
                return EVP_sha1();
            case crypto::hash_algo::ALGO_SHA256:
                return EVP_sha256();
            case crypto::hash_algo::ALGO_SHA384:
                return EVP_sha384();
            default:
                // not reached unless we forget to add a case!
                throw crypto::hash_exception("unknown algorithm: " + std::to_string(algo));
        }
    }
}
