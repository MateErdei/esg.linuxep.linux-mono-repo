///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <include/gtest/gtest.h>

#include "versig.h"

namespace
{
    TEST(versig_test, no_args) // NOLINT
    {
        std::unique_ptr<char> name(strdup("versig_test"));
        char *argv[] = {name.get(), nullptr};
        int argc = sizeof(argv) / sizeof(char*) - 1;
        int ret = versig_main(argc, argv);
        EXPECT_EQ(ret,2);
    }

    TEST(versig_test, test_valid) // NOLINT
    {
        std::vector<std::string> argv{"versig_test", "-c../tests/cert_files/rootca.crt.valid" ,"-f../tests/data_files/manifest.dat.valid"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret,0);
    }
}

