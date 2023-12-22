// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/OSUtilities/ISystemUtils.h"

#include <memory>

namespace OSUtilities
{
    class SystemUtilsImpl : public ISystemUtils
    {
    public:
        std::string getEnvironmentVariable(const std::string& key) const override;
    };

    std::unique_ptr<ISystemUtils>& systemUtilsStaticPointer();
} // namespace OSUtilitiesImpl