/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace sophos_on_access_process::onaccessimpl
{
    class ClientSocketException : virtual public std::runtime_error
    {
    public:
        explicit ClientSocketException(const std::string& errorMsg) : std::runtime_error(errorMsg) {}
    };
}
