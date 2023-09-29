// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "hash.h"

#include <string>
#include <vector>

namespace manifest
{
    struct signature
    {
        std::string::size_type body_length_;  // the amount of body verified by this signature
        crypto::hash_algo algo_;              // the signature algorithm (SHA1, SHA256, etc)
        std::string signature_;               // the signature
        std::string certificate_;             // the signing certificate
        std::vector<std::string> cert_chain_; // the intermediate certificates
    };
}
