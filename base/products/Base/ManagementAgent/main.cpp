// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"

#include "ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h"

static int management_agent_main(int argc, char* argv[])
{
    return ManagementAgent::ManagementAgentImpl::ManagementAgentMain::main(argc, argv);
}

MAIN(management_agent_main(argc, argv))
