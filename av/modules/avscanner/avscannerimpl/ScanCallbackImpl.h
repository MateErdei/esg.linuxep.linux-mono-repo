/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseFileWalkCallbacks.h"
#include "ScanClient.h"

namespace avscanner::avscannerimpl
{
    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override;
        void infectedFile(const path& p, const std::string& threatName, bool isSymlink=false) override;
        void scanError(const std::string& errorMsg) override;

        [[nodiscard]] int returnCode() const { return m_returnCode; }

    private:
        int m_returnCode = E_CLEAN;
    };
}