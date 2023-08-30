// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

namespace Common::UpdateUtilities
{
    std::vector<std::string> listOfAllPreviousReports(const std::string& outputParentPath);
}
