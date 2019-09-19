/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Logger.h"

#include <Common/Threads/AbstractThread.h>

#include <cassert>
#include <cstring>
#include <mutex>
#include <sstream>

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
            size_t m_outputLimit;
            size_t m_outputSize;
            std::function<void(void)> m_notifyFinished;
            std::function<void(std::string)> m_outputTrimmed;
        public:
            /**
             *
             * @param fileDescriptor BORROWED fileDescriptor
             */
            StdPipeThread(int fileDescriptor,
                    std::function<void(void)> notifyFinished);
            explicit StdPipeThread(int fileDescriptor);

            ~StdPipeThread() override;

            std::string output();

            bool hasFinished()
            {
                // use lock in order to ensure ::run has finished. Hence, child process finished.
                std::unique_lock<std::mutex> lock(m_mutex);
                return true;
            }
            void setOutputLimit(size_t limit) { m_outputLimit = limit; }
            void setOutputTrimmedCallBack(std::function<void(std::string)> outputTrimmed);
        private:
            void run() override;

        protected:
            void trimStream();
        };
    } // namespace ProcessImpl
} // namespace Common
