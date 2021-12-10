/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanSerialiser.h"

#include <NamedScan.capnp.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace manager::scheduler;

std::string ScanSerialiser::serialiseScan(const ScheduledScanConfiguration& config,
                                                      const ScheduledScan& nextScan)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::NamedScan>();

    requestBuilder.setName(nextScan.name());
    requestBuilder.setScanArchives(nextScan.archiveScanning());
    requestBuilder.setScanImages(nextScan.archiveScanning());
    requestBuilder.setScanAllFiles(config.scanAllFileExtensions());
    requestBuilder.setScanFilesWithNoExtensions(config.scanFilesWithNoExtensions());
    requestBuilder.setScanHardDrives(nextScan.hardDrives());
    requestBuilder.setScanCDDVDDrives(nextScan.CDDVDDrives());
    requestBuilder.setScanNetworkDrives(nextScan.networkDrives());
    requestBuilder.setScanRemovableDrives(nextScan.removableDrives());

    {
        auto exclusionsInput = config.exclusions();
        auto exclusions = requestBuilder.initExcludePaths(exclusionsInput.size());
        for (unsigned i = 0; i < exclusionsInput.size(); i++)
        {
            exclusions.set(i, exclusionsInput[i]);
        }
    }

    {
        auto extensionExclusionsInput = config.sophosExtensionExclusions();
        auto extensionExclusions = requestBuilder.initSophosExtensionExclusions(extensionExclusionsInput.size());
        for (unsigned i = 0; i < extensionExclusionsInput.size(); ++i)
        {
            extensionExclusions.set(i, extensionExclusionsInput[i]);
        }
    }

    {
        auto extensionInclusionsInput = config.userDefinedExtensionInclusions();
        auto extensionInclusions = requestBuilder.initUserDefinedExtensionInclusions(extensionInclusionsInput.size());
        for (unsigned i = 0; i < extensionInclusionsInput.size(); ++i)
        {
            extensionInclusions.set(i, extensionInclusionsInput[i]);
        }
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}
