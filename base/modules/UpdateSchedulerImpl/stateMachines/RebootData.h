// Copyright 2020 Sophos Limited.

#pragma once

#include "TieWithCheck.h"

#include <chrono>
#include <cstdint>

namespace RebootData
{
    // Underlying type is unsigned 32-bit to map directly to DWORD registry value.
    enum class RequiredEnum : std::uint32_t
    {
        // These are persisted in the registry; do NOT reassign values different meanings.
        // Higher value indicates greater significance; do NOT reorder.
        // The values map directly to those of the REBOOTCODE in the COM interface (IALUpdateNotification); do not change.
        RebootNotRequired = 0, // The zero value is the default (i.e. used when registry value is missing).
        RebootOptional = 1,
        RebootMandatory = 2,
    };

    // The reboot-specific data that's included in status messages to Central.
    struct Status
    {
        bool requiredFlag{};
        std::chrono::system_clock::time_point requiredSince{};
        bool urgent{};
    };

    inline auto TieMembers(const Status & o)
    {
        return CppCommon::TieWithCheck<Status>(o.requiredFlag, o.requiredSince, o.urgent);
    }

    MAKE_STRICT_TOTAL_ORDER(Status);

    struct MachineState
    {
        RequiredEnum requiredEnum{};
        std::chrono::system_clock::time_point requiredSince{};

        // (credit == 0) => reboot is urgent
        // (credit > 0) => reboot is not urgent
        // Default credit value gives 2-weeks worth of normal hourly updates.
        // For an endpoint that operates 8 hours per day and hibernates for 16 hours per day that translates to 6 weeks.
        std::uint32_t credit{ 14 * 24 };
    };

    inline auto TieMembers(const MachineState & o)
    {
        return CppCommon::TieWithCheck<MachineState>(o.requiredEnum, o.requiredSince, o.credit);
    }

    MAKE_STRICT_TOTAL_ORDER(MachineState);
} // namespace RebootData
