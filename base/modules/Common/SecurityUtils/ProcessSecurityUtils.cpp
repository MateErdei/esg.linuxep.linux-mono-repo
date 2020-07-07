/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessSecurityUtils.h"

#include <iostream>
#include <cstdlib>
#include <grp.h>


namespace Common::SecurityUtils
{
    /*
     * drop privileges permanently, effective if the process is root.
     * params: git_t newgid, uid_t newuid (force to use real non-privileged user)
     * Ref: https://learning.oreilly.com/library/view/secure-programming-cookbook/0596003943/ch01s03.html
     */
    void dropPrivileges(gid_t newgid, uid_t newuid) {
        gid_t oldgid = getegid();
        uid_t olduid = geteuid();
        /* Pare down the ancillary groups for the process before doing anything else because the setgroups()
         * system call requires root privileges.
         * Ref: https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
         */
        std::cout << "inside dropPrivilege: " << newgid << " " << newuid << std::endl;


        if (!olduid && setgroups(1, &newgid) == -1)
        {
            std::cout << "inside dropPrivilege old: " << oldgid << " " << olduid << std::endl;
            abort();
        }

        if (newgid != oldgid && (setregid(newgid, newgid) == -1)) {
            abort();
        }

        if (newuid != olduid && (setreuid(newuid, newuid) == -1)) {
            std::cout << "Failed to set euid" << std::endl;
            abort();
        }
        /* verify that the changes were successful */
        if (newgid != oldgid && (setegid(oldgid) != -1 || getegid() != newgid)) {
            abort();
        }
        if (newuid != olduid && (seteuid(olduid) != -1 || geteuid() != newuid)) {
            abort();
        }
    }


    /*
     * Must be called after user look up and before privilege drop
     */
    int setupJailAndGoIn(const std::string &chrootDirPath)
    {
        if (chdir(chrootDirPath.c_str()))
        {
            std::cout << "failed to chdir to jail" << std::endl;
            exit(EXIT_FAILURE);
        }
        if (chroot(chrootDirPath.c_str())) {
            std::cout << "failed to chroot to jail" << std::endl;
            exit(EXIT_FAILURE);
        }
        if(setenv("PWD", "/", 1 ))
        {
            std::cout << "failed to sync PWD" << std::endl;
            exit(EXIT_FAILURE);
        }
        return EXIT_SUCCESS;
    }

    passwd *getUserToRunAs(const std::string &runAsUser) {
        struct passwd *userent = nullptr;
        if (geteuid() == 0 && !runAsUser.empty()) {
            userent = getpwnam(runAsUser.c_str());
            if (!userent)
                std::cout << "failed to get user" << std::endl;
        }
        return userent;
    }



    uid_t _get_uid_by_name(const std::string &user_name)
    {
        auto buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
        char *buf = static_cast<char *>(malloc(buflen));
        struct passwd *pwbufp;
        struct passwd pwbuf;
        getpwnam_r(user_name.c_str(), &pwbuf, buf, buflen, &pwbufp);
        free(buf);
        return pwbufp? pwbufp->pw_uid : -1;
    }


    void chrootAndDropPrivileges(const std::string &runAsUser, const std::string &chrootDirPath)
    {
        auto userEntry = getUserToRunAs(runAsUser);
        if (!userEntry) {
            exit(EXIT_FAILURE);
        }
        if (EXIT_SUCCESS != setupJailAndGoIn(chrootDirPath)) {
            exit(EXIT_FAILURE);
        }
        dropPrivileges(userEntry->pw_gid, userEntry->pw_uid);
    }
}