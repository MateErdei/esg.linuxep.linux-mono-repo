// Copyright 2023-2024 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "NTPPolicy.h"
#include "PluginCallback.h"
#include "TaskQueue.h"

#include "deviceisolation/modules/plugin/INftWrapper.h"

#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PersistentValue/PersistentValue.h"

namespace Plugin
{
    class PluginAdapter
    {
    public:
        PluginAdapter(
                std::shared_ptr<TaskQueue> queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                std::shared_ptr<PluginCallback> callback);
        void mainLoop();

    TEST_PUBLIC:
        void enableIsolation(const INftWrapperPtr& nftWrapper);
        void disableIsolation(const INftWrapperPtr& nftWrapper);


    private:
        void processAction(const std::string& actionXml);
        void processPolicy(const std::string& appId, const std::string& policyXml);
        void updateIsolationRules(const std::shared_ptr<NTPPolicy> newPolicy);
        void sendStatus();

        std::shared_ptr<TaskQueue> taskQueue_;
        std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseService_;
        std::shared_ptr<PluginCallback> callback_;
        std::shared_ptr<NTPPolicy> ntpPolicy_;
        INftWrapperPtr nftWrapper_;
        Common::PersistentValue<bool> isolationEnabled_;

    protected:
        static constexpr std::chrono::milliseconds DEFAULT_QUEUE_TIMEOUT = std::chrono::seconds{5};
        static constexpr std::chrono::milliseconds DEFAULT_DEBUG_MISSING_POLICY_TIMEOUT = std::chrono::seconds{5};
        static constexpr std::chrono::milliseconds DEFAULT_WARN_MISSING_POLICY_TIMEOUT = std::chrono::seconds{60};
        // Timeout to use when we don't need to wake up again
        static constexpr std::chrono::milliseconds MAXIMUM_TIMEOUT = std::chrono::seconds{600};
        // Mutable so that unit tests can change it
        std::chrono::milliseconds queueTimeout_ = DEFAULT_QUEUE_TIMEOUT;
        std::chrono::milliseconds debugMissingPolicyTimeout_ = DEFAULT_DEBUG_MISSING_POLICY_TIMEOUT;
        std::chrono::milliseconds warnMissingPolicyTimeout_ = DEFAULT_WARN_MISSING_POLICY_TIMEOUT;

    };
} // namespace Plugin
