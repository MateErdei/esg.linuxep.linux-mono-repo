/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
// break the convention on namespace to make easier to use around all the project.
namespace sophos
{
    std::string user();

    std::string localUser();

    std::string group();

    /*
     * Local low privilege user
     */
    std::string localUser();

    /*
     * Local group
     */
    std::string localGroup();

    /*
     * Update Scheduler user to run the Update Scheduler component as low privilege
     */
    std::string updateSchedulerUser();

} // namespace sophos
