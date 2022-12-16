// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"

class IDetectionHandler
{
public:
    virtual ~IDetectionHandler() = default;

    /**
     * Handles a detection
     * Sets the correlationId in the detection object
     * @param detection
     */
    virtual void finaliseDetection(scan_messages::ThreatDetected& detection) = 0;

    /**
     * To be called before finaliseDetection if quarantine is attempted for the detection.
     * Only a single detection may be marked as being quarantined at a time, before being resolved with
     * finaliseDetection. Once the result of quarantining is known, finaliseDetection can be called.
     * Sets the correlationId in the detection object which can be used with e.g. SafeStore
     * @param detection
     */
    virtual void markAsQuarantining(scan_messages::ThreatDetected& detection) = 0;
};
