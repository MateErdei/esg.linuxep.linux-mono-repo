/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ISophosSplUsers.h"

#include <string>

// break the convention on namespace to make easier to use around all the project.
namespace sophos
{
    class SophosSplUsers : public ISophosSplUsers
    {
    public:
        [[nodiscard]] std::string splUser() const override;

        [[nodiscard]] std::string splGroup() const override;

        /*
         * Local low privilege user
         */
        [[nodiscard]] std::string splLocalUser() const override;

        /*
         * Local group
         */
        [[nodiscard]] std::string splLocalGroup() const override;
    };

} // namespace sophos
