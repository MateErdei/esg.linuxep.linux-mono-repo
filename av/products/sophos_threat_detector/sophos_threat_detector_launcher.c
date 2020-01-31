/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/prctl.h>

#define pr_arg(x) ((unsigned long) x)

static int cap_set_ambient(cap_value_t cap, cap_flag_value_t set)
{
    int result, val;
    switch (set)
    {
        case CAP_SET:
            val = PR_CAP_AMBIENT_RAISE;
            break;
        case CAP_CLEAR:
            val = PR_CAP_AMBIENT_LOWER;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    result = prctl(PR_CAP_AMBIENT, pr_arg(val), pr_arg(cap),  pr_arg(0), pr_arg(0));
    return result;
}

static int pass_on_chroot_capability()
{
    cap_flag_value_t v = 0;
    cap_t caps = cap_get_proc();
    int ret;

    if (caps == NULL)
    {
        perror("Failed to get caps");
        return 30;
    }
    cap_value_t newcaps[1] = { CAP_SYS_CHROOT, };
    ret = cap_set_flag(caps, CAP_INHERITABLE, 1, newcaps, CAP_SET);
    if (ret != 0)
    {
        perror("Failed to cap_set_flag");
        return 31;
    }
    ret = cap_set_proc(caps);
    if (ret != 0)
    {
        perror("Failed to cap_set_proc");
        return 32;
    }
    ret = cap_free(caps);
    if (ret != 0)
    {
        perror("Failed to free cap");
        return 33;
    }

    errno = 0;
    ret = cap_set_ambient(CAP_SYS_CHROOT, CAP_SET);
    if (ret != 0)
    {
        perror("cap_set_ambient fails");
        return 34;
    }

    return 0;
}

int main()
{
    char* argv[2] = {"sophos_threat_detector", NULL};
    int ret = pass_on_chroot_capability();
    if (ret != 0)
    {
        return ret;
    }
    execv("/opt/sophos-spl/plugins/av/sbin/sophos_threat_detector", argv);
    return 70; // If the exec fails
}
