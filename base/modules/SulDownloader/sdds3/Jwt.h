// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISignatureVerifierWrapper.h"

#include <nlohmann/json.hpp>
#include <string>

namespace SulDownloader
{
    [[nodiscard]] nlohmann::json verifyAndLoadJwtPayload(
        const ISignatureVerifierWrapper& verifier,
        const std::string& jwt);
} // namespace SulDownloader
