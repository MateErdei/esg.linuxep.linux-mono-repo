/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>

static int management_agent_main(int argc, char* argv[])
{
    return ManagementAgent::ManagementAgentImpl::ManagementAgentMain::main(argc, argv);
}

MAIN(management_agent_main(argc, argv))
