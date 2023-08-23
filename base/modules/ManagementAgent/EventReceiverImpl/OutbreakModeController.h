// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOutbreakModeController.h"

#include <chrono>

#ifndef TEST_PUBLIC
#    define TEST_PUBLIC private
#endif

namespace ManagementAgent::EventReceiverImpl
{
    class OutbreakModeController : public IOutbreakModeController
    {
    public:
        OutbreakModeController();
        bool processEvent(const Event& event) override;
        [[nodiscard]] bool outbreakMode() const override;
        void processAction(const std::string& actionXml) override;

        TEST_PUBLIC : using clock_t = std::chrono::system_clock;
        using time_point_t = std::chrono::time_point<clock_t>;
        bool processEvent(const Event& event, time_point_t now);

    private:
        static std::string generateUUID();
        std::string generateCoreOutbreakEvent(const std::string& timestamp);
        void save(const std::string& timestamp);
        void load();
        void resetCountOnDayChange(time_point_t now);
        void leaveOutbreakMode(time_point_t now);

    protected:
        std::string uuid_;
        int detectionCount_ = 0;
        int savedYear_ = 0;
        int savedMonth_ = -1;
        int savedDay_ = 0;
        bool outbreakMode_ = false;
    };

    using OutbreakModeControllerPtr = std::shared_ptr<OutbreakModeController>;
} // namespace ManagementAgent::EventReceiverImpl
