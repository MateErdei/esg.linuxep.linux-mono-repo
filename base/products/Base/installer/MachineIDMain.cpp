// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Installer/MachineId/MachineId.h"

#include "Common/Main/Main.h"

static int machineid_main(int argc, char* argv[])
{
    return Installer::MachineId::mainEntry(argc, argv);
}

MAIN(machineid_main(argc, argv))