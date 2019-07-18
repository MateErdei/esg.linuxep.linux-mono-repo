/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ReadableFd.h"

#include "Logger.h"

#include <Common/UtilityImpl/StrError.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

namespace Common
{
    namespace ReactorImpl
    {
        ReadableFd::ReadableFd(int fd, bool closeOnDestructor) : m_fd(fd), m_closeOnDestructor(closeOnDestructor) {}

        ReadableFd::~ReadableFd()
        {
            if (m_closeOnDestructor)
            {
                close();
            }
        }

        Common::ZeroMQWrapper::IReadable::data_t ReadableFd::read()
        {
            std::string currvalue;
            char buffer[31];
            while (true)
            {
                auto nbytes = ::read(m_fd, buffer, 30);
                if (nbytes == -1)
                {
                    LOGERROR("Read from file descriptor reported error: " << Common::UtilityImpl::StrError(errno));
                    break;
                }
                buffer[nbytes] = '\0';
                currvalue += buffer;
                // when reading from pipe, the number of bytes may be smaller than
                // the count (30). And this is not an error. But signal, that there is no more data to be read right
                // now.
                if (nbytes < 30)
                {
                    break;
                }
            }
            Common::ZeroMQWrapper::IReadable::data_t data;
            data.emplace_back(currvalue);
            return data;
        }

        int ReadableFd::fd() { return m_fd; }

        void ReadableFd::close()
        {
            if (m_fd >= 0)
            {
                ::close(m_fd);
                m_fd = -1;
            }
        }

        void ReadableFd::release()
        {
            m_fd = -1;
            m_closeOnDestructor = false;
        }
    } // namespace ReactorImpl
} // namespace Common