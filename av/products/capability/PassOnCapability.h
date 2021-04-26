/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SSPL_PLUGIN_MAV_PASSONCAPABILITY_H
#define SSPL_PLUGIN_MAV_PASSONCAPABILITY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/capability.h>

int pass_on_capability(cap_value_t cap);

void set_no_new_privs();

#ifdef __cplusplus
} // extern "C"
#endif

#endif //SSPL_PLUGIN_MAV_PASSONCAPABILITY_H
