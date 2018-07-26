/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef COMMON_PROCESSIMPL_PIPEHOLDER_H
#define COMMON_PROCESSIMPL_PIPEHOLDER_H

#include <Common/Process/IProcessException.h>

#include <cassert>

#include <unistd.h>

namespace Common
{
    namespace ProcessImpl
    {
        class PipeHolder
        {
        public:
            PipeHolder() : m_pipe{-1, -1}, m_pipeclosed{false, false}
            {
                if (::pipe(m_pipe) < 0)
                {
                    throw Common::Process::IProcessException("Pipe construction failure");
                }
                m_pipeclosed[PIPE_READ] = false;
                m_pipeclosed[PIPE_WRITE] = false;
            }

            ~PipeHolder()
            {
                if (!m_pipeclosed[PIPE_READ])
                {
                    ::close(m_pipe[PIPE_READ]);
                }
                if (!m_pipeclosed[PIPE_WRITE])
                {
                    close(m_pipe[PIPE_WRITE]);
                }
            }

            int readFd()
            {
                assert(!m_pipeclosed[PIPE_READ]);
                return m_pipe[PIPE_READ];
            }

            int writeFd()
            {
                assert(!m_pipeclosed[PIPE_WRITE]);
                return m_pipe[PIPE_WRITE];

            }

            void closeRead()
            {
                assert(!m_pipeclosed[PIPE_READ]);
                close(m_pipe[PIPE_READ]);
                m_pipeclosed[PIPE_READ] = true;
            }

            void closeWrite()
            {
                assert(!m_pipeclosed[PIPE_WRITE]);
                close(m_pipe[PIPE_WRITE]);
                m_pipeclosed[PIPE_WRITE] = true;
            }


        private:
            int m_pipe[2];
            bool m_pipeclosed[2];
            constexpr static int PIPE_READ = 0;
            constexpr static int PIPE_WRITE = 1;


        };


    }
}

#endif //COMMON_PROCESSIMPL_PIPEHOLDER_H
