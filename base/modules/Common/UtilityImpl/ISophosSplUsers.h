// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>

namespace sophos
{
    // use an inteface to allow unit testing with different users
    class ISophosSplUsers
    {
    public:
        /*
         * main sophos spl user
         */
        [[nodiscard]] virtual std::string splUser() const = 0;

        /*
         * main sophos spl group
         */
        [[nodiscard]] virtual std::string splGroup() const = 0;

        /*
         * Local low privilege user
         */
        [[nodiscard]] virtual std::string splLocalUser() const = 0;

        /*
         * Local group
         */
        [[nodiscard]] virtual std::string splLocalGroup() const = 0;
    };

} // namespace sophos
