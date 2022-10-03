// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ThreatDetected.h"

#include <ThreatDetected.capnp.h>

namespace scan_messages
{
    class ServerThreatDetected : public ThreatDetected
    {
    public:
        explicit ServerThreatDetected(Sophos::ssplav::ThreatDetected::Reader& reader);
        [[nodiscard]] std::string getFilePath() const;
        [[nodiscard]] std::string getThreatName() const;
        [[nodiscard]] bool hasFilePath() const;
        [[nodiscard]] std::int64_t getDetectionTime() const;
        [[nodiscard]] std::string getUserID() const;
        [[nodiscard]] E_SCAN_TYPE getScanType() const;
        [[nodiscard]] E_NOTIFCATION_STATUS getNotificationStatus() const;
        [[nodiscard]] E_THREAT_TYPE getThreatType() const;
        [[nodiscard]] E_ACTION_CODE getActionCode() const;
        [[nodiscard]] std::string getSha256() const;
        [[nodiscard]] std::string getThreatId() const;
    };
}



