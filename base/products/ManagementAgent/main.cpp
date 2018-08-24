/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO);  //Read and write for the owner
    return ManagementAgent::ManagementAgentImpl::ManagementAgentMain::main(argc,argv);
}
