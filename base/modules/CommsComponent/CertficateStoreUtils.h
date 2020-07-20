/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <optional>

namespace CommsComponent
{
    std::optional<std::string> getCertificateStorePath();

    std::optional<std::string> getCaCertificateBundleFile();
}  //CommsComponent
