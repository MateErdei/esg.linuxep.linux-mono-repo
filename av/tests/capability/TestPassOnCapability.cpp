/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "capability/PassOnCapability.h"

#include "datatypes/Print.h"

#include <gtest/gtest.h>

#include <unistd.h>

TEST(TestPassOnCapability, pass_on_capability_root_only) // NOLINT
{
    if (::getuid() != 0)
    {
        return;
    }

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_EQ(ret, 0);
}


static void drop_root()
{
    PRINT("Drop root");
    int ret = setregid(1, 1);
    ASSERT_EQ(ret, 0);
    ret = setreuid(1, 1);
    ASSERT_EQ(ret, 0);
}


TEST(TestPassOnCapability, pass_on_capability_no_root) // NOLINT
{
    if (::getuid() == 0)
    {
        drop_root();
    }

    ASSERT_NE(::getuid(), 0);

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_NE(ret, 0);

    // non-root process returns 62 - cap_set_proc fails
    // ex-root process returns 64 - cap_set_ambient fails - cap_set_proc works

//    EXPECT_EQ(ret, E_CAP_SET_PROC);
}

TEST(TestPassOnCapability, no_new_privs) // NOLINT
{
    set_no_new_privs();
}