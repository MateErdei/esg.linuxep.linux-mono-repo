// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "products/capability/PassOnCapability.h"

#include "datatypes/Print.h"

#include <gtest/gtest.h>

#include <unistd.h>

TEST(TestPassOnCapability, pass_on_capability_root_only)
{
    if (::getuid() != 0)
    {
        return;
    }

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_EQ(ret, 0);
}

#ifndef SPL_BAZEL
# define DROP_ROOT
#endif

#ifdef DROP_ROOT
static void drop_root()
{
    PRINT("Drop root");
    int ret = setregid(1, 1);
    ASSERT_EQ(ret, 0);
    ret = setreuid(1, 1);
    ASSERT_EQ(ret, 0);
}
#endif

TEST(TestPassOnCapability, pass_on_capability_no_root)
{
    if (::getuid() == 0)
    {
#ifdef DROP_ROOT
        drop_root();
#else
        GTEST_SKIP() << "Unable to handle testing pass_on_capability_no_root as root under bazel";
#endif
    }

    ASSERT_NE(::getuid(), 0);

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_NE(ret, 0);

    // non-root process returns 62 - cap_set_proc fails
    // ex-root process returns 64 - cap_set_ambient fails - cap_set_proc works

//    EXPECT_EQ(ret, E_CAP_SET_PROC);
}

TEST(TestPassOnCapability, no_new_privs)
{
    set_no_new_privs();
}