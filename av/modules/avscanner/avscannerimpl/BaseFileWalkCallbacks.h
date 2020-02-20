/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <filewalker/IFileWalkCallbacks.h>
#include "ScanClient.h"

namespace avscanner::avscannerimpl
{
    class BaseFileWalkCallbacks : public filewalker::IFileWalkCallbacks
    {
    protected:
        explicit BaseFileWalkCallbacks(ScanClient scanner);
        ScanClient m_scanner;

    public:
        BaseFileWalkCallbacks(const BaseFileWalkCallbacks&) = delete;
        BaseFileWalkCallbacks(BaseFileWalkCallbacks&&) = delete;
        virtual ~BaseFileWalkCallbacks() = default;
    };
}



