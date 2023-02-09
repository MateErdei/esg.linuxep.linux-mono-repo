// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "IActionsImpl.h"
namespace ResponseActionsImpl
{
    class UploadFileAction: public IActionsImpl
    {
    public:
        std::string run(const std::string& actionJson) const override;
    };

}