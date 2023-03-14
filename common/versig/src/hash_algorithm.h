// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

namespace crypto
{
    // The algorithm enum is also a bitmask for the hash constructor (which algorithms to calculate)
    // Must be ordered by strength - lowest == weakest
    enum hash_algo
    {
        ALGO_MD5 = 0b00000001,
        ALGO_SHA1 = 0b00000010,
        ALGO_SHA256 = 0b00000100,
        ALGO_SHA384 = 0b00001000,
        ALGO_SHA512 = 0b00010000,
    };

    constexpr int ALL_HASH_ALGORITHMS =
        ALGO_MD5 | ALGO_SHA1 | ALGO_SHA256 | ALGO_SHA384 | ALGO_SHA512;

    constexpr int SECURE_HASH_ALGORTHMS = ALGO_SHA256 | ALGO_SHA384 | ALGO_SHA512;
}