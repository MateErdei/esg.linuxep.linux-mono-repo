// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

namespace Installer::MachineId
{
    /**
     * Receives as first argument SOPHOS_INSTALL.
     * Verify if the machine id exists. If it does not, create.
     *
     * @param argc
     * @param argv: program-name , sophos_install
     * @return 0 for success, otherwise error code
     */
    int mainEntry(int argc, char* argv[]);
} // namespace Installer::MachineId
