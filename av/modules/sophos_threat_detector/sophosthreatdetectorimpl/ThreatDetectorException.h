// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "datatypes/AVException.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorException : public datatypes::AVException
    {
    public:
        using datatypes::AVException::AVException;
    };

    class SusiUpdateFailedException : public ThreatDetectorException
    {
    public:
        using ThreatDetectorException::ThreatDetectorException;
    };
}
