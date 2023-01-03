// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/ApplicationPaths.h"
#include "common/StatusFile.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class OnAccessStatusFile : public common::StatusFile
    {
    public:
        OnAccessStatusFile()
            : common::StatusFile(Plugin::getOnAccessStatusPath())
        {}
    };
}
