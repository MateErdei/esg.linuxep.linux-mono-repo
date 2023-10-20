// Copyright 2023 Sophos Limited. All rights reserved.

#include "MetadataRescan.h"

#include "scan_messages/MetadataRescan.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

namespace scan_messages
{
    std::string MetadataRescan::Serialise() const
    {
        ::capnp::MallocMessageBuilder messageBuilder;

        Sophos::ssplav::MetadataRescan::Builder metadataRescanBuilder =
            messageBuilder.initRoot<Sophos::ssplav::MetadataRescan>();

        metadataRescanBuilder.setThreatType(threat.type);
        metadataRescanBuilder.setThreatName(threat.name);
        metadataRescanBuilder.setThreatSha256(threat.sha256);
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
            .filePath = metadataRescanReader.getFilePath(),
            .sha256 = metadataRescanReader.getSha256(),
            .threat = {
                .type = metadataRescanReader.getThreatType(),
                .name = metadataRescanReader.getThreatName(),
                .sha256 = metadataRescanReader.getThreatSha256(),
            },
        };
    }

    bool MetadataRescan::operator==(const MetadataRescan& other) const
    {
        return threat == other.threat && filePath == other.filePath && sha256 == other.sha256;
    }
} // namespace scan_messages
