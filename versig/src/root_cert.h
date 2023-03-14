//=============================================================================
// Copyright 2021 Sophos Limited. All rights reserved.
//
// 'Sophos' and 'Sophos Anti-Virus' are registered trademarks of Sophos Limited and Sophos Group.
// All other product and company names mentioned are trademarks or registered trademarks of their
// respective owners.
//=============================================================================

#pragma once

#include <string>

namespace crypto
{
    struct root_cert
    {
        std::string pem_crt;
        std::string pem_crl;
    };
} // namespace crypto