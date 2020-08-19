/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>

namespace avscanner::mountinfo
{
    class ISystemPaths
    {
    public:
        virtual ~ISystemPaths() = default;
        [[nodiscard]] virtual std::string mountInfoFilePath() const = 0;
        [[nodiscard]] virtual std::string cmdlineInfoFilePath() const = 0;
        [[nodiscard]] virtual std::string findfsCmdPath() const = 0;
        [[nodiscard]] virtual std::string mountCmdPath() const = 0;
    };

    using ISystemPathsSharedPtr = std::shared_ptr<ISystemPaths>;
}
