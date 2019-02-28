/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServer.h"

#include <iostream>
int main(int argc, char* argv[])
{
    try
    {
        auto context = Common::ZMQWrapperApi::createContext();

        if (argc == 2)
        {
            FakeServer fakeServer(argv[1], true);

            fakeServer.run(*context);
            fakeServer.join();
        }
        else
        {
            std::cerr << "not enough arguments passed" << std::endl;
        }

        return 0;
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}