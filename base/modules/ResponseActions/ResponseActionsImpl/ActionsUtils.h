// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "ActionStructs.h"

#include <string>
namespace ResponseActionsImpl
{
    class ActionsUtils
    {
    public:


        UploadInfo readUploadAction(const std::string& actionJson, UploadType type);
        bool isExpired(u_int64_t expiry);
    };
}

