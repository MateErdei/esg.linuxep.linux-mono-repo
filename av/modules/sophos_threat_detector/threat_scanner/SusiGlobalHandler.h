/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <memory>

namespace threat_scanner
{
    class SusiGlobalHandler
    {
    public:
        explicit SusiGlobalHandler(const std::string& json_config);
        ~SusiGlobalHandler();

        bool update(const std::string& path);
    };
    using SusiGlobalHandlerSharePtr = std::shared_ptr<SusiGlobalHandler>;
}