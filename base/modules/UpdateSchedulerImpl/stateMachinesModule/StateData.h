/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TieWithCheck.h"

#include <chrono>
#include <cstdint>

namespace StateData
{
    enum class DownloadResultEnum
    {
        good,
        bad,
        noNetwork,
    };

    // Underlying type is unsigned 32-bit to map directly to DWORD registry value.
    enum class StatusEnum : std::uint32_t
    {
        // These are persisted in the registry; do NOT reassign values different meanings.
        good = 0, // The zero value is the default (i.e. used if registry value cannot be read).
        bad = 1,
    };


    ///// The data for status messages to Central /////

    struct DownloadStatus
    {
        StatusEnum status{};
        std::chrono::system_clock::time_point failedSince{};
    };
    inline auto TieMembers(const DownloadStatus & o)
    {
        return CppCommon::TieWithCheck<DownloadStatus>(o.status, o.failedSince);
    }
    MAKE_STRICT_TOTAL_ORDER(DownloadStatus);

    struct InstallStatus
    {
        StatusEnum status{};

        std::chrono::system_clock::time_point lastGood{};
        std::chrono::system_clock::time_point failedSince{};
    };
    inline auto TieMembers(const InstallStatus & o)
    {
        return CppCommon::TieWithCheck<InstallStatus>(o.status, o.lastGood, o.failedSince);
    }
    MAKE_STRICT_TOTAL_ORDER(InstallStatus);


    ///// Internal data for the state machines /////

    struct DownloadMachineState
    {
        // (credit == 0) => bad
        // ((credit > 0) && (credit < defaultCredit)) => good (assuming temporary failure)
        // (credit == defaultCredit) => good
        static constexpr auto defaultStandardCredit = 4;
        static constexpr auto noNetworkCreditMultiplicationFactor = 18;
        static constexpr auto defaultCredit = defaultStandardCredit * noNetworkCreditMultiplicationFactor;
        std::uint32_t credit{ defaultCredit };

        std::chrono::system_clock::time_point failedSince{};
    };
    inline auto TieMembers(const DownloadMachineState & o)
    {
        return CppCommon::TieWithCheck<DownloadMachineState>(o.credit, o.failedSince);
    }
    MAKE_STRICT_TOTAL_ORDER(DownloadMachineState);

    struct InstallMachineState
    {
        // (credit == 0) => bad
        // ((credit > 0) && (credit < defaultCredit)) => good (assuming temporary failure)
        // (credit == defaultCredit) => good
        static constexpr auto defaultCredit = 3;
        std::uint32_t credit{ defaultCredit };

        std::chrono::system_clock::time_point lastGood{};
        std::chrono::system_clock::time_point failedSince{};
    };
    inline auto TieMembers(const InstallMachineState & o)
    {
        return CppCommon::TieWithCheck<InstallMachineState>(o.credit, o.lastGood, o.failedSince);
    }
    MAKE_STRICT_TOTAL_ORDER(InstallMachineState);

    struct EventMachineState
    {
        static constexpr auto throttlePeriod = std::chrono::hours{ 24 };

        int lastError{};
        std::chrono::system_clock::time_point lastTime{};
    };
    inline auto TieMembers(const EventMachineState & o)
    {
        return CppCommon::TieWithCheck<EventMachineState>(o.lastError, o.lastTime);
    }
    MAKE_STRICT_TOTAL_ORDER(EventMachineState);
} // namespace StateData
