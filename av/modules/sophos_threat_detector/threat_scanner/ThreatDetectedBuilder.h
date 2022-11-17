// Copyright 2022, Sophos Limited. All rights reserved.

#pragma once

#include "ScanResult.h"

#include "scan_messages/ThreatDetected.h"

namespace threat_scanner
{
    std::string generateThreatId(const std::string& filePath, const std::string& threatName);

    /**
     * Gets the main detection for a path from a list of detections.
     * Requires at least one detection to be provided.
     * If no detection exists for the exact path provided, then this function returns a new detection, with the name and
     * type of the first provided detection, and calculates the SHA256 using the file descriptor. We do this for the
     * case of archives, where there can be multiple detections for inner files that have paths that differ from the
     * path of the archive, e.g. /tmp/archive.zip/eicar.txt compared to /tmp/archive.zip.
     * Currently we can only report one detection per file (hence also per archive).
     *
     * In the future this will be replaced once we can report multiple detections per file, see LINUXDAR-5933.
     */
    threat_scanner::Detection getMainDetection(
        const std::vector<threat_scanner::Detection>& detections,
        const std::string& path,
        int fd);

    /**
     * Builds a ThreatDetected object from a list of detections, scan path and file descriptor.
     * Requires at least one detection to be provided.
     */
    scan_messages::ThreatDetected buildThreatDetected(
        const std::vector<threat_scanner::Detection>& detections,
        const std::string& path,
        datatypes::AutoFd autoFd,
        const std::string& userId,
        scan_messages::E_SCAN_TYPE scanType);
} // namespace threat_scanner