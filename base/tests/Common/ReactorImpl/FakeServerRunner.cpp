/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServer.h"
#include <iostream>
int main(int argc, char * argv[])
{
    try{
        auto context = Common::ZeroMQWrapper::createContext();

        FakeServer fakeServer(argv[1], true);

        fakeServer.run(*context);
        fakeServer.join();
        return 0;
    }catch (std::exception & ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}