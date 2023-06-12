// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/ProcessImpl/ProcessImpl.h"

namespace
{
    class ProcessReplacement
    {
    public:
        explicit ProcessReplacement(std::function<std::unique_ptr<Common::Process::IProcess>()> functor)
        {
            Common::ProcessImpl::ProcessFactory::instance().replaceCreator(std::move(functor));
        }
        ~ProcessReplacement() { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }
    };
}
