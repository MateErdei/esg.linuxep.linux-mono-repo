// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <stddef.h>

#include <unordered_set>
#include <string>



namespace sophos_on_access_process::OnAccessConfig
{
   //Event Reading Queue
   const unsigned long int onAccessProcessFdLimit = 1048576;

   const size_t maxAllowedQueueSize = 1048000;
   const size_t minAllowedQueueSize = 1000;
   const size_t defaultMaxScanQueueSize = 4000;

    //FileSystem
    const std::unordered_set<std::string> FILE_SYSTEMS_TO_EXCLUDE
    {
           "aufs",
           "autofs",
           "bpf",
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
    const bool defaultDumpPerfData = false;

    //Scanning Threads
    const int maxAllowedScanningThreads = 100;
    const int minAllowedScanningThreads = 1;
    const int defaultScanningThreads = 10;


}