/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PlatformUtils.h"
#include "OSUtilities/IPlatformUtilsException.h"
#include <unistd.h>
#include <cerrno>
#include <sys/utsname.h>
#include <cstring>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        std::string PlatformUtils::getHostname() const
        {
            PlatformUtils::checkUtsname();
            struct utsname buffer;
            return buffer.nodename;
//            size_t const maxLenOfHostname = 255;
//            char *hostname = new char[maxLenOfHostname];
//            int returnCode = gethostname(hostname, maxLenOfHostname);
//
//            if (returnCode != 0)
//            {
//                throw OSUtilities::IPlatformUtilsException("Unable to gethostname due to: " << std::strerror(errno));
//            }
//            std::string result(hostname);
//            return result;
        }

        std::string PlatformUtils::getPlatform() const
        {
            PlatformUtils::checkUtsname();
            struct utsname buffer;
            return buffer.sysname;
        }

        std::string PlatformUtils::getVendor() const
        {
            return "";
        }

        std::string PlatformUtils::getArchitecture() const
        {
            PlatformUtils::checkUtsname();
            struct utsname buffer;
            return buffer.machine;
        }

        void PlatformUtils::checkUtsname() const
        {
            struct utsname buffer;
            errno = 0;
            if (uname(&buffer) < 0)
            {
                perror("uname");
                throw OSUtilities::IPlatformUtilsException("uname call failed: " + std::string(strerror(errno)));
            }
        }
    }
}