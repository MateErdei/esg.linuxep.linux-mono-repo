/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Logger.h"

#include <Common/Threads/AbstractThread.h>

#include <cstring>
#include <sstream>
#include <mutex>
#include <cassert>

namespace Common
{
    namespace ProcessImpl
    {
// Pipe has a limited capacity (see man 7 pipe : Pipe Capacity)
// This class allows the pipe buffer to be cleared to ensure that child process is never blocked trying
// to write to stdout or stderr.
        class StdPipeThread : public Common::Threads::AbstractThread
        {
        private:
            int m_fileDescriptor;
            std::stringstream m_stdoutStream;
            std::mutex m_mutex;
        public:
            /**
             *
             * @param fileDescriptor BORROWED fileDescriptor
             */
            explicit StdPipeThread(int fileDescriptor);

            ~StdPipeThread() override = default;

            std::string output()
            {
                hasFinished();
                return m_stdoutStream.str();
            }

            bool hasFinished()
            {
                // use lock in order to ensure ::run has finished. Hence, child process finished.
                std::unique_lock <std::mutex> lock(m_mutex);
                return true;
            }

        private:
            void run() override;



        };
    }
}
