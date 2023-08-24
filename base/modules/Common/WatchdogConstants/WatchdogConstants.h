// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace Common::WatchdogConstants
{
    namespace Actions
    {
        inline const std::string& TriggerUpdate()
        {
            static const std::string trigger{ "TriggerUpdate" };
            return trigger;
        }
        inline const std::string& TriggerDiagnose()
        {
            static const std::string trigger{ "TriggerDiagnose" };
            return trigger;
        }
    } // namespace Actions

    inline std::string WatchdogServiceLineName()
    {
        return "watchdogservice";
    }
} // namespace Common::WatchdogConstants
