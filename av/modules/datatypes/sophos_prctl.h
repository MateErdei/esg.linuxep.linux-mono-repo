// Copyright 2023 Sophos Limited. All rights reserved.
// Used from C code!
#ifndef ESG_LINUXEP_LINUX_MONO_REPO_SOPHOS_PRCTL_H
#define ESG_LINUXEP_LINUX_MONO_REPO_SOPHOS_PRCTL_H

#include <sys/prctl.h>

// These are not defined with our toolchain, not even with crosstool-ng
#ifndef PR_CAP_AMBIENT
#define PR_CAP_AMBIENT 47
#endif
#ifndef PR_CAP_AMBIENT_RAISE
#define PR_CAP_AMBIENT_RAISE 2
#endif
#ifndef PR_CAP_AMBIENT_LOWER
#define PR_CAP_AMBIENT_LOWER 3
#endif
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS	38
#endif

#endif //ESG_LINUXEP_LINUX_MONO_REPO_SOPHOS_PRCTL_H
