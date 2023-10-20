// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ScanResponse.capnp.h"

#include <string>
#include <vector>

namespace scan_messages
{
    struct Detection
    {
        std::string path;
        std::string type;
        std::string name;
        std::string sha256;
    };

    class ScanResponse
    {
    public:
        ScanResponse();
        explicit ScanResponse(Sophos::ssplav::FileScanResponse::Reader reader);

        void addDetection(
            const std::string& filePath,
            const std::string& threatType,
            const std::string& threatName,
            const std::string& sha256);
        void setErrorMsg(std::string errorMsg);

        [[nodiscard]] std::vector<Detection> getDetections();

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] bool allClean();

        [[nodiscard]] std::string getErrorMsg();

    private:
        std::vector<Detection> m_detections;
        std::string m_errorMsg;
    };
} // namespace scan_messages
