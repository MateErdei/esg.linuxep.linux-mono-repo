// Copyright 2023 Sophos Limited. All rights reserved.

#include "MetadataRescan.h"

#include "MetadataRescan.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

namespace scan_messages
{
    std::string MetadataRescan::Serialise() const
    {
        ::capnp::MallocMessageBuilder messageBuilder;

        Sophos::ssplav::MetadataRescan::Builder metadataRescanBuilder =
            messageBuilder.initRoot<Sophos::ssplav::MetadataRescan>();

        metadataRescanBuilder.setThreatType(threatType);
        metadataRescanBuilder.setThreatName(threatName);
        metadataRescanBuilder.setFilePath(filePath);
        metadataRescanBuilder.setSha256(sha256);

        kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(messageBuilder);
        kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
        std::string dataAsString(bytes.begin(), bytes.end());

        return dataAsString;
    }


    MetadataRescan MetadataRescan::Deserialise(const kj::ArrayPtr<const capnp::word> protoBuffer)
    {
        capnp::FlatArrayMessageReader messageReader(protoBuffer);
        Sophos::ssplav::MetadataRescan::Reader metadataRescanReader =
            messageReader.getRoot<Sophos::ssplav::MetadataRescan>();

        return MetadataRescan{
            .threatType = metadataRescanReader.getThreatType(),
            .threatName = metadataRescanReader.getThreatName(),
            .filePath = metadataRescanReader.getFilePath(),
            .sha256 = metadataRescanReader.getSha256(),
        };
    }

    bool MetadataRescan::operator==(const MetadataRescan& other) const
    {
        // clang-format off
        return threatType == other.threatType &&
               threatName == other.threatName &&
               filePath == other.filePath &&
               sha256 == other.sha256;
        // clang-format on
    }
} // namespace scan_messages
