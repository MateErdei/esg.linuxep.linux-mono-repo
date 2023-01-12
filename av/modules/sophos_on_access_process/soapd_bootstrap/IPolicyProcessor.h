// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

namespace sophos_on_access_process::soapd_bootstrap
{
    class IPolicyProcessor
    {
    public:
        virtual ~IPolicyProcessor() = default;

        virtual void ProcessPolicy() = 0;
    };
}