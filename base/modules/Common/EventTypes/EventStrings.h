/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace EventTypes
    {
        static const std::string PortScanningEventName = "Detector.PortScanning";
        static const std::string CredentialEventName = "Detector.Credentials";
        static const std::string ProcessEventName = "Detector.Process";
        static const std::string AnyDetectorName = "Detector";
    }
}