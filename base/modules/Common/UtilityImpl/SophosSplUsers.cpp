/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SophosSplUsers.h"

#include "ProjectNames.h"

namespace sophos
{
    std::string SophosSplUsers::splUser() const { return sophos::user(); }

    std::string SophosSplUsers::splGroup() const { return sophos::group(); }

    std::string SophosSplUsers::splNetworkUser() const { return sophos::networkUser(); }

    std::string sophos::SophosSplUsers::splNetworkGroup() const { return sophos::networkGroup(); }

    std::string sophos::SophosSplUsers::splLocalUser() const { return sophos::localUser(); }

    std::string sophos::SophosSplUsers::splLocalGroup() const { return sophos::localGroup(); }
} // namespace sophos