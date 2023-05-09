// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Threat.h"

#include <capnp/common.h>

#include <memory>
#include <string>

namespace scan_messages
{
    struct MetadataRescan
    {
        [[nodiscard]] bool operator==(const MetadataRescan& other) const;

        [[nodiscard]] std::string Serialise() const;
        [[nodiscard]] static MetadataRescan Deserialise(const kj::ArrayPtr<const capnp::word> protoBuffer);

        std::string filePath;
        std::string sha256;
        Threat threat;
    };

    // This is the response sent back for metadata rescans
    // It is not serialised into a capnproto object, but directly into a byte written to the socket
    // The response does not have any other data sent down the socket
    enum class MetadataRescanResponse : uint8_t
    {
        undetected,
        clean,
        threatPresent,
        needsFullScan,
        failed
    };

    inline constexpr const char* MetadataRescanResponseToString(MetadataRescanResponse response)
    {
        switch (response)
        {
            case MetadataRescanResponse::undetected:
                return "undetected";
            case MetadataRescanResponse::clean:
                return "clean";
            case MetadataRescanResponse::threatPresent:
                return "threat present";
            case MetadataRescanResponse::needsFullScan:
                return "needs full scan";
            case MetadataRescanResponse::failed:
                return "failed";
        }
        return "unknown";
    }
} // namespace scan_messages