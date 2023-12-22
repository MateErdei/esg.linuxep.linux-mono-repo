// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/OSUtilities/ISystemUtils.h"

#include <memory>
#include <mutex>

namespace Tests
{
    void replaceSystemUtils(std::unique_ptr<OSUtilities::ISystemUtils>);
    void restoreSystemUtils();
    struct ScopedReplaceSystemUtils{
        std::lock_guard<std::mutex> m_guard;
        ScopedReplaceSystemUtils(std::unique_ptr<OSUtilities::ISystemUtils>);
        ScopedReplaceSystemUtils();
        void replace(std::unique_ptr<OSUtilities::ISystemUtils>);
        ~ScopedReplaceSystemUtils();
    };
} // namespace Tests
