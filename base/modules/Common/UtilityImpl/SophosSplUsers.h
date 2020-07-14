/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include "ISophosSplUsers.h"

// break the convention on namespace to make easier to use around all the project.
namespace sophos
{
    class SophosSplUsers : public ISophosSplUsers
    {
    public:
        [[nodiscard]] std::string splUser() const override;

        [[nodiscard]] std::string splGroup() const override;

        /*
         * Comms component network facing low privilege user
         */
        [[nodiscard]] std::string splNetworkUser() const override;

        /*
         * Comms component network facing group
         */
        [[nodiscard]] std::string splNetworkGroup() const override;

        /*
         * Comms component local low privilege user
         */
        [[nodiscard]] std::string splLocalUser() const override;

        /*
         * Comms component local group
         */
        [[nodiscard]] std::string splLocalGroup() const override;
    };

} // namespace Common
