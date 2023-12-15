// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SusRequestParameters.h"

#include "Common/XmlUtilities/AttributesMap.h"

namespace SulDownloader
{
    std::string createSUSRequestBody(const SUSRequestParameters& parameters);

} // namespace SulDownloader
