/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ZeroMQWrapper/IReadable.h"
namespace Common
{
    namespace ReactorImpl
    {
        class ReadableFd : virtual public Common::ZeroMQWrapper::IReadable
        {
        public:
            ReadableFd(int fd, bool closeOnDestructor);
            ~ReadableFd();

            Common::ZeroMQWrapper::IReadable::data_t read() override;
            int fd() override;
            void close();
            void release();

        private:
            int m_fd;
            bool m_closeOnDestructor;
        };
    } // namespace ReactorImpl
} // namespace Common
