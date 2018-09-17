/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MACinfo.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>

namespace
{
    struct SocketRAII
    {
        SocketRAII(int socketfd_) : socketfd(socketfd_)
        {
            if ( socketfd_ == -1 )
            {
                throw std::system_error( errno, std::generic_category(), "Failed to create socket");
            }
        }
        int socketfd;
        ~SocketRAII()
        {
            ::close(socketfd);
        }
    };
}

namespace Common
{
    namespace OSUtilities
    {
        using MACType = std::array<unsigned char, 6>;

        std::string stringfyMAC( const MACType & macAddress)
        {
            std::stringstream s;
            for( int i=0; i<6; i++)
            {
                if( i != 0)
                {
                    s << ':';
                }
                s << std::hex << (int) macAddress[i];
            }
            return s.str();
        }

        std::vector<std::string> sortedSystemMACs()
        {
            std::vector<std::string> macs;
            struct ifreq ifr;
            struct ifconf ifc;
            std::array<char, 1024> buf;

            SocketRAII sock( socket(AF_INET, SOCK_DGRAM, IPPROTO_IP) );

            ifc.ifc_len = buf.size();
            ifc.ifc_buf = buf.data();
            // get ifaces list
            if (ioctl(sock.socketfd, SIOCGIFCONF, &ifc) == -1)
            {
                throw std::system_error( errno, std::generic_category(), "Failed to get information for the interfaces configured.");
            }

            struct ifreq* it = ifc.ifc_req;
            const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

            for (; it != end; ++it) {
                strcpy(ifr.ifr_name, it->ifr_name);
                if (ioctl(sock.socketfd, SIOCGIFFLAGS, &ifr) == 0)
                {
                    if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                        if (ioctl(sock.socketfd, SIOCGIFHWADDR, &ifr) == 0)
                        {
                            MACType mac_address;
                            memcpy(mac_address.data(), ifr.ifr_hwaddr.sa_data, mac_address.size());
                            macs.push_back(stringfyMAC(mac_address));
                        }
                    }
                }
                else
                {
                    throw std::system_error( errno, std::generic_category(), "Failed to get information mac address.");
                }
            }

            std::sort( std::begin(macs), std::end(macs));
            return macs;
        }

    }
}