// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include <cstddef>
#include <unordered_set>
#include <string>

namespace sophos_on_access_process::local_settings
{
   //Event Reading Queue
   constexpr unsigned long int onAccessProcessFdLimit = 1048576;

   constexpr size_t maxAllowedQueueSize = 1048000;
   constexpr size_t minAllowedQueueSize = 1000;
   constexpr size_t defaultMaxScanQueueSize = 100000;

    //FileSystem
    const std::unordered_set<std::string> FILE_SYSTEMS_TO_EXCLUDE
    {
           "aufs",
           "autofs",
           "bpf",
           "cephfs",
           "efivarfs",
           "fuse.gvfsd-fuse",
           "fuse.vmware-vmblock",
           "inotifyfs",
           "nssadmin",
           "nsfs",
           "nsspool",
           "romfs",
           "rootfs",
           "subfs",
           "usbdevfs",
    };

    //Performance
    constexpr bool defaultDumpPerfData = false;

    constexpr bool defaultCacheAllEvents = false;

    //Scanning Threads
    constexpr int maxAllowedScanningThreads = 100;
    constexpr int minAllowedScanningThreads = 1;
    constexpr int defaultScanningThreads = 10;


}