// Copyright 2023 Sophos Limited. All rights reserved.

#include "file_info.h"

#include <utility>

#include "hash.h"

namespace manifest
{
    using namespace std;

    file_info::file_info() : size_ {}, algorithms_ {}
    {
    }

    file_info::file_info(path_t path, unsigned long size, std::string sha1) :
        path_(std::move(path)), size_(size), sha1_(std::move(sha1)), algorithms_(crypto::hash_algo::ALGO_SHA1)
    {
    }

    file_info& file_info::sha256(const std::string& sha256)
    {
        sha256_ = sha256;
        algorithms_ |= crypto::hash_algo::ALGO_SHA256;
        return *this;
    }

    file_info& file_info::sha384(const std::string& sha384)
    {
        sha384_ = sha384;
        algorithms_ |= crypto::hash_algo::ALGO_SHA384;
        return *this;
    }

}