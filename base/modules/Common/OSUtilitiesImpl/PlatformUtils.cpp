/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PlatformUtils.h"

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "OSUtilities/IPlatformUtilsException.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <map>
#include <unistd.h>
#include <ctype.h>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        PlatformUtils::PlatformUtils()
            : m_vendor("UNKNOWN")
            , m_osName("")
            , m_osMajorVersion("")
            , m_osMinorVersion("")
        {
            populateVendorDetails();
        }
        
        void PlatformUtils::populateVendorDetails()
        {
            const std::string lsbReleasePath = "/etc/lsb-release";

            const std::array<std::string, 5> distroCheckFiles = {
                lsbReleasePath,
                "/etc/issue",
                "/etc/centos-release",
                "/etc/redhat-release",
                "/etc/system-release"
            };

            std::map<std::string, std::string> distroNames = {
                std::make_pair("redhat", "redhat"),
                std::make_pair("ubuntu", "ubuntu"),
                std::make_pair("centos", "centos"),
                std::make_pair("amazonlinux", "amazon")
            };
            
            auto fs = FileSystem::fileSystem();
            for(auto& path : distroCheckFiles)
            {
                if(fs->isFile(path))
                {
                    std::string distro;
                    std::vector<std::string> fileContents = fs->readLines(path);
                    if(!fileContents.empty())
                    {
                        distro = fileContents[0];
                        distro = UtilityImpl::StringUtils::replaceAll(distro, " ", "");
                        distro = UtilityImpl::StringUtils::replaceAll(distro, "/", "_");
                        std::transform(distro.begin(), distro.end(), distro.begin(),
                                       [](unsigned char c){ return std::tolower(c); });
                        std::string tempDistro = distroNames[distro];
                        if(!tempDistro.empty())
                        {
                            m_vendor = tempDistro;
                            break;
                        }
                    }
                }
            }

            if(fs->isFile(lsbReleasePath))
            {
                m_osName = UtilityImpl::StringUtils::extractValueFromIniFile(lsbReleasePath, "DISTRIB_DESCRIPTION");
                std::string version = UtilityImpl::StringUtils::extractValueFromIniFile(lsbReleasePath, "DISTRIB_RELEASE");
                std::vector<std::string> majorAndMinor = UtilityImpl::StringUtils::splitString(version, ".");
                if(majorAndMinor.size() >= 2)
                {
                    m_osMajorVersion = majorAndMinor[0];
                    m_osMinorVersion = majorAndMinor[1];
                }
            }
        }

        std::string PlatformUtils::getHostname() const
        {
            return PlatformUtils::getUtsname().nodename;
        }

        std::string PlatformUtils::getPlatform() const
        {
            return PlatformUtils::getUtsname().sysname;
        }

        std::string PlatformUtils::getVendor() const
        {
            return m_vendor;
        }

        std::string PlatformUtils::getArchitecture() const
        {
            // example machine value is x86_64
            return PlatformUtils::getUtsname().machine;
        }

        std::string PlatformUtils::getOsName() const
        {
            return m_osName;
        }

        std::string PlatformUtils::getKernelVersion() const
        {
            return PlatformUtils::getUtsname().release;
        }

        std::string PlatformUtils::getOsMajorVersion() const
        {
            return m_osMajorVersion;
        }

        std::string PlatformUtils::getOsMinorVersion() const
        {
            return m_osMinorVersion;
        }

        std::string PlatformUtils::getDomainname() const
        {
            return "UNKNOWN";
        }

        utsname PlatformUtils::getUtsname() const
        {
            utsname platformInfo;
            errno = 0;
            if (uname(&platformInfo) < 0)
            {
                perror("uname");
                throw OSUtilities::IPlatformUtilsException("uname call failed: " + std::string(strerror(errno)));
            }
            return platformInfo;
        }
    }
}