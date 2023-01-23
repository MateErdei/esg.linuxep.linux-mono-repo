// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IPolicyProcessor.h"

#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class OnAccessProcessControlCallback : public unixsocket::IProcessControlMessageCallback
    {
    public:
        explicit OnAccessProcessControlCallback(
            sophos_on_access_process::soapd_bootstrap::IPolicyProcessor& policyProcessor) :
            m_policyProcessor(policyProcessor)
        {
        }

        void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) override;

    private:
        void ProcessPolicy();

        sophos_on_access_process::soapd_bootstrap::IPolicyProcessor& m_policyProcessor;
    };
} // namespace sophos_on_access_process::OnAccessConfig
