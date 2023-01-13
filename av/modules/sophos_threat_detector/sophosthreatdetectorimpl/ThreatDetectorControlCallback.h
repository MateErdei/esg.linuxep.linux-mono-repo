// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include "ISophosThreatDetectorMain.h"

#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorControlCallback : public unixsocket::IProcessControlMessageCallback
    {
    public:
        explicit ThreatDetectorControlCallback(ISophosThreatDetectorMainPtr mainInstance);

        void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) override;

    private:
        ISophosThreatDetectorMainPtr m_mainInstance;
    };
}
