//
// Created by pair on 25/06/18.
//

#ifndef EVEREST_BASE_READABLEFD_H
#define EVEREST_BASE_READABLEFD_H
#include "Common/ZeroMQWrapper/IReadable.h"
namespace Common
{
    namespace ReactorImpl
    {
        class ReadableFd: virtual public Common::ZeroMQWrapper::IReadable
        {
        public:
            ReadableFd( int fd , bool closeOnDestructor);
            ~ReadableFd();

            Common::ZeroMQWrapper::IReadable::data_t read() override ;
            int fd() override ;
            void close();
            void release();

        private:
            int m_fd;
            bool m_closeOnDestructor;
        };
    }
}




#endif //EVEREST_BASE_READABLEFD_H
