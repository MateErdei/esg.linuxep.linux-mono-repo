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
     * Comms component network facing low privilege user
     */
    std::string networkUser();

    /*
     * Comms component network facing group
     */
    std::string networkGroup();

    /*
     * Comms component local low privilege user
     */
    std::string localUser();

    /*
     * Comms component local group
     */
    std::string localGroup();

    /*
     * Update Scheduler user to run the Update Scheduler component as low privilege
     */
    std::string updateSchedulerUser();

    /*
 * Update Scheduler user to run the Update Scheduler component as low privilege
 */
    std::string sduUser();
} // namespace sophos
