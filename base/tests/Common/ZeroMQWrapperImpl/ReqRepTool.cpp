/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ReqRepTestImplementations.h"

#include <iostream>

using namespace ReqRepTest;

int unreliableReplier(const std::string& serveraddress, const std::string& killch, const std::string& action)
{
    UnreliableReplier ur(serveraddress, killch);
    if (action == "serveRequest")
    {
        ur.serveRequest();
    }
    else if (action == "breakAfterReceiveMessage")
    {
        ur.breakAfterReceiveMessage();
    }
    else if (action == "servePartialReply")
    {
        ur.servePartialReply();
    }
    else
    {
        std::cerr << "Unknown action: " << action << std::endl;
        return 2;
    }
    return 0;
}

int unreliableRequester(
    const std::string& serveraddress,
    const std::string& killch,
    const std::string& action,
    const std::string& value)
{
    UnreliableRequester ur(serveraddress, killch);
    if (action == "breakAfterSendRequest")
    {
        ur.breakAfterSendRequest(value);
    }
    else if (action == "sendReceive")
    {
        ur.sendReceive(value);
    }
    else
    {
        std::cerr << "Unknown action: " << action << std::endl;
        return 2;
    }

    return 0;
}

static int main_inner(int argc, char* argv[])
{
    std::cerr << "ReqRepTool PID=" << ::getpid() << std::endl;
    for (int i = 0; i < argc; i++)
    {
        std::cerr << i << " " << argv[i] << std::endl;
    }
    if (argc < 5)
    {
        std::cerr << "Syntax: ReqRepTool <serveraddress> <killch> <actor> <action> [<args>*]" << std::endl;
        return 2;
    }
    std::string serveraddress = argv[1];
    std::string killch = argv[2];
    std::string actor = argv[3];
    std::string action = argv[4];

    if (actor == "UnreliableReplier")
    {
        return unreliableReplier(serveraddress, killch, action);
    }
    else if (actor == "UnreliableRequester")
    {
        assert(argc > 5);
        return unreliableRequester(serveraddress, killch, action, argv[5]);
    }
    std::cerr << "Unknown actor: " << actor << std::endl;
    return 2;
}

int main(int argc, char* argv[])
{
    try
    {
        return main_inner(argc, argv);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Uncaught exception at top-level: " << ex.what() << std::endl;
        return 80;
    }
}
