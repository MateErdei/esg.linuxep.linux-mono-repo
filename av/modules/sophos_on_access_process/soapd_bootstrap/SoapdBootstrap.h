// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "IOnAccessService.h"
#include "ISoapdResources.h"
#include "OnAccessRunner.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        explicit SoapdBootstrap(ISoapdResources& soapdResources);
        SoapdBootstrap(const SoapdBootstrap&) =delete;
        SoapdBootstrap& operator=(const SoapdBootstrap&) =delete;

        static int runSoapd();
        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */

TEST_PUBLIC:
        int outerRun();

    private:
        void innerRun();

        ISoapdResources& m_soapdResources;

        std::unique_ptr<OnAccessRunner> m_onAccessRunner;
        service_impl::IOnAccessServicePtr m_ServiceImpl;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;
    };
}