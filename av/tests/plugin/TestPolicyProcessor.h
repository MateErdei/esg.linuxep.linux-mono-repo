// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "pluginimpl/PolicyProcessor.h"

#include "datatypes/Print.h"

namespace
{
    class PolicyProcessorUnitTestClass : public Plugin::PolicyProcessor
    {
    public:
        PolicyProcessorUnitTestClass()
            : Plugin::PolicyProcessor(nullptr)
        {}
    protected:
        void notifyOnAccessProcessIfRequired() override
        {
            PRINT("Notified soapd");
        }
    };
}
