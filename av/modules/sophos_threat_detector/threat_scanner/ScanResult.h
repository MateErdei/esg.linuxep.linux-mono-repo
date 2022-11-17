// Copyright 2022, Sophos Limited. All rights reserved.

#pragma once

#include <log4cplus/loglevel.h>

#include <string>
#include <vector>

namespace threat_scanner
{
    struct Detection
    {
        std::string path;
        std::string name;
        std::string type;
        std::string sha256;

        bool operator==(const Detection& other) const
        {
            return path == other.path && name == other.name && type == other.type && sha256 == other.sha256;
        }
    };

    struct ScanError
    {
        std::string message;
        log4cplus::LogLevel logLevel;
    };

    struct ScanResult
    {
        std::vector<Detection> detections;
        std::vector<ScanError> errors;
    };
} // namespace threat_scanner