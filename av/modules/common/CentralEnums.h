// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <stdexcept>

namespace common
{
    namespace CentralEnums
    {
        // Defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42233512399/EMP+event-core-detection
        enum class ThreatType
        {
            unknown = 0,
            virus = 1, // historical name, represents all malware
            pua = 2,
            // ? = 3,
            suspiciousBehaviour = 4,
            suspiciousFile = 5,
            malwareOutbreak = 6,
            block = 7,
            amsiProtection = 8
        };

        // Defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42233512399/EMP+event-core-detection
        // Also referenced from ReportSource.h in SSP
        enum ReportSource
        {
            ml = 0,
            // sav = 1, // No longer supported in SSP
            rep = 2,
            blocklist = 3,
            // appwl = 4, // No longer supported in SSP
            amsi = 5,
            ips = 6,
            behavioral = 7,
            vdl = 8,
            hbt = 9,
            mtd = 10,

            // These are specified in ReportSource.h in SSP, but not in the EMP
            web = 11,
            deviceControl = 12,
            hmpa = 13,
            webControl = 14
        };

        // Defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42233512399/EMP+event-core-detection
        enum class Origin
        {
            mlMalwareDetection = 0,
            vdlMalwareDetection = 1,
            mlPuaDetection = 2,
            vdlPuaDetection = 3,
            hmpaDetection = 4,
            mtdDetection = 5,
            hbtDetection = 6,

            // In SSP these are actually called 'Scan Now UI' and 'Scan Now Central' respectively
            // And seem mostly to only be used for internal reporting and keeping track of health
            // We might not need to use these, as they should be used only if they are the direct origin of a report
            scanNow = 7,
            scheduledScan = 8,

            reputationMalwareDetection = 9,
            reputationPuaDetection = 10,
            blocklistedByAdmin = 11,
            amsiDetection = 12,
            ipsDetection = 13,
            behavioral = 14
        };

        // Code based on InteractionProtocolHelper::GetOriginOf in SSP
        constexpr Origin getOriginOf(ReportSource source, ThreatType type)
        {
            switch (source)
            {
                case ReportSource::ml:
                    return ThreatType::pua == type ? Origin::mlPuaDetection : Origin::mlMalwareDetection;
                case ReportSource::vdl:
                    return ThreatType::pua == type ? Origin::vdlPuaDetection : Origin::vdlMalwareDetection;
                case ReportSource::rep:
                    if (type == ThreatType::pua)
                    {
                        return Origin::reputationPuaDetection;
                    }
                    if (type == ThreatType::virus)
                    {
                        return Origin::reputationMalwareDetection;
                    }
                    throw std::runtime_error(
                        "Unexpected threat type for reputation source" + std::to_string(static_cast<int>(type)));
                case ReportSource::blocklist:
                    return Origin::blocklistedByAdmin;
                case ReportSource::amsi:
                    return Origin::amsiDetection;
                case ReportSource::ips:
                    return Origin::ipsDetection;
                case ReportSource::hbt:
                    return Origin::hbtDetection;
                case ReportSource::mtd:
                    return Origin::mtdDetection;
                case ReportSource::behavioral:
                    return Origin::behavioral;
                case ReportSource::web:
                    return Origin::mtdDetection;
                case ReportSource::deviceControl:
                    // Unknown, not specified in SSP
                    break;
                case ReportSource::hmpa:
                    return Origin::hmpaDetection;
                case ReportSource::webControl:
                    // Unknown, not specified in SSP
                    break;
            }

            throw std::runtime_error("Unexpected source: " + std::to_string(static_cast<int>(source)));
        }

        static_assert(Origin::mlMalwareDetection == getOriginOf(ReportSource::ml, ThreatType::virus));
        static_assert(Origin::mlPuaDetection == getOriginOf(ReportSource::ml, ThreatType::pua));
        static_assert(Origin::vdlMalwareDetection == getOriginOf(ReportSource::vdl, ThreatType::virus));
        static_assert(Origin::vdlPuaDetection == getOriginOf(ReportSource::vdl, ThreatType::pua));
        static_assert(Origin::reputationMalwareDetection == getOriginOf(ReportSource::rep, ThreatType::virus));
        static_assert(Origin::reputationPuaDetection == getOriginOf(ReportSource::rep, ThreatType::pua));
        static_assert(Origin::blocklistedByAdmin == getOriginOf(ReportSource::blocklist, ThreatType::virus));
        static_assert(Origin::amsiDetection == getOriginOf(ReportSource::amsi, ThreatType::amsiProtection));
        static_assert(Origin::ipsDetection == getOriginOf(ReportSource::ips, ThreatType::virus));
        static_assert(Origin::hbtDetection == getOriginOf(ReportSource::hbt, ThreatType::virus));
        static_assert(Origin::mtdDetection == getOriginOf(ReportSource::mtd, ThreatType::virus));
        static_assert(Origin::behavioral == getOriginOf(ReportSource::behavioral, ThreatType::virus));
        static_assert(Origin::mtdDetection == getOriginOf(ReportSource::web, ThreatType::virus));
        static_assert(Origin::hmpaDetection == getOriginOf(ReportSource::hmpa, ThreatType::virus));
    } // namespace CentralEnums
} // namespace common