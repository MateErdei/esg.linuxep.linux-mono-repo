// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZMQWrapperApi/IContext.h"

// These are made available for unit tests
namespace Common::WatchdogRequestImpl
{
    void requestUpdateService(Common::ZMQWrapperApi::IContext& context);
    void requestDiagnoseService(Common::ZMQWrapperApi::IContext& context);
} // namespace Common::WatchdogRequestImpl
