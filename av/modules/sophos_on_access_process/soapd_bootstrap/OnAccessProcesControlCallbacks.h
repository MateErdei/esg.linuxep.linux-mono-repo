// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "SoapdBootstrap.h"

#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

namespace sophos_on_access_process::OnAccessConfig
{
    class OnAccessProcessControlCallback : public unixsocket::IProcessControlMessageCallback
    {
    public:
        explicit OnAccessProcessControlCallback(sophos_on_access_process::soapd_bootstrap::SoapdBootstrap& soapd)
            : m_soapd(soapd)
        {}

        void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) override;

    private:
        void ProcessPolicy();

        sophos_on_access_process::soapd_bootstrap::SoapdBootstrap& m_soapd;
    };
}
