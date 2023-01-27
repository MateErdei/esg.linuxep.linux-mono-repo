// Copyright 2023 Sophos Limited.  All rights reserved.

#pragma once

#include "IPolicyProcessor.h"

#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include <utility>

namespace sophos_on_access_process::soapd_bootstrap
{
    class IOnAccessRunner : public IPolicyProcessor
    {
    public:
        virtual ~IOnAccessRunner() = default;

        virtual void setupOnAccess() = 0;
        virtual void enableOnAccess() = 0;
        virtual void disableOnAccess() = 0;

        virtual timespec* getTimeout() = 0;
        virtual void onTimeout() = 0;

        virtual threat_scanner::IUpdateCompleteCallbackPtr getUpdateCompleteCallbackPtr() = 0;
    };
} // namespace sophos_on_access_process::soapd_bootstrap
