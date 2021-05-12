/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H
#define SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H

#define TOTAL_MAX_RECONNECTS 200
#define MAX_CONN_RETRIES 20
#ifdef USING_LIBFUZZER
#define MAX_SCAN_RETRIES 1
#else
#define MAX_SCAN_RETRIES 20
#endif

#endif // SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H
