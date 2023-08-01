// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include <SulDownloader/sdds3/SDDS3Repository.h>

namespace SulDownloader
{
    std::string createSUSRequestBody(const SUSRequestParameters& parameters);
    bool isValidChar(char c);

} // namespace SulDownloader
