// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include "ISophosThreatDetectorMain.h"

#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorControlCallback : public unixsocket::IProcessControlMessageCallback
    {
    public:
        explicit ThreatDetectorControlCallback(ISophosThreatDetectorMain& mainInstance);

        void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) override;

    private:
        ISophosThreatDetectorMain& m_mainInstance;
    };
}
