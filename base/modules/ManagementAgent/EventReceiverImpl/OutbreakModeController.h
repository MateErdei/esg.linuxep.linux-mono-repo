// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOutbreakModeController.h"

#include <chrono>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace ManagementAgent::EventReceiverImpl
{
    class OutbreakModeController : public IOutbreakModeController
    {
    public:
        OutbreakModeController();
        bool recordEventAndDetermineIfItShouldBeDropped(const std::string& appId, const std::string& eventXml) override;
    TEST_PUBLIC:
        using clock_t = std::chrono::system_clock;
        using time_point_t = std::chrono::time_point<clock_t>;
        bool recordEventAndDetermineIfItShouldBeDropped(const std::string& appId,
                                                        const std::string& eventXml,
                                                        time_point_t now
                                                        );
        bool outbreakMode() const;

    private:
        void save();
        void load();
        void resetCountOnDayChange(time_point_t now);
        int detectionCount_ = 0;
        int savedYear_ = 0;
        int savedMonth_ = -1;
        int savedDay_ = 0;
        bool outbreakMode_ = false;
    };

    using OutbreakModeControllerPtr = std::shared_ptr<OutbreakModeController>;
}
