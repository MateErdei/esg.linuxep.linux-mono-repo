/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PassOnCapability.h"

#include "common/ErrorCodesC.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/prctl.h>

#define pr_arg(x) ((unsigned long) x)

static int cap_set_ambient(cap_value_t cap, cap_flag_value_t set)
{
    int val;
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
    int result = prctl(PR_CAP_AMBIENT, pr_arg(val), pr_arg(cap),  pr_arg(0), pr_arg(0));
    return result;
}

int pass_on_capability(cap_value_t cap)
{
    cap_t caps = cap_get_proc();
    int ret;

    if (caps == NULL)
    {
        perror("Failed to get caps");
        return E_CAP_GET_PROC;
    }
    cap_value_t newcaps[1] = { cap, };
    ret = cap_set_flag(caps, CAP_INHERITABLE, 1, newcaps, CAP_SET);
    if (ret != 0)
    {
        perror("Failed to cap_set_flag");
        cap_free(caps);
        return E_CAP_SET_FLAG;
    }
    ret = cap_set_proc(caps);
    if (ret != 0)
    {
        perror("Failed to cap_set_proc");
        cap_free(caps);
        return E_CAP_SET_PROC;
    }
    ret = cap_free(caps);
    if (ret != 0)
    {
        perror("Failed to free cap");
        return E_CAP_FREE;
    }

    errno = 0;
    ret = cap_set_ambient(cap, CAP_SET);
    if (ret != 0)
    {
        perror("cap_set_ambient fails");
        return E_CAP_SET_AMBIENT;
    }

    return 0;
}

void set_no_new_privs()
{
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
}
