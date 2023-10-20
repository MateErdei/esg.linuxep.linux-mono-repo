// Copyright 2023 Sophos All rights reserved.

#include "Susi.h"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        return 2;
    }
    const auto& installSourceFolder = "/opt/sophos-spl/plugins/av/chroot/susi/update_source";
    const auto& installDestination = argv[1];
    auto ret = ::SUSI_Install(installSourceFolder, installDestination);

    if (ret == SUSI_E_INVALIDARG)
    {
        std::cerr << "Failure: SUSI_E_INVALIDARG\n";
    }
    else
    {
        std::cerr << "Result: 0x" << std::hex << (unsigned long)ret << '\n';
        std::cerr << "Failure: " << SUSI_FAILURE(ret) << '\n';
    }

    return 0;
}
