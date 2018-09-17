/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef COMMON_PROCESS_IPROCESS_H
#define COMMON_PROCESS_IPROCESS_H

#include "EnvPair.h"

#include <chrono>
#include <string>
#include <memory>
#include <vector>

namespace Common
{
    namespace Process
    {
        using Milliseconds = std::chrono::milliseconds;
        enum class ProcessStatus {FINISHED, TIMEOUT, RUNNING};
        Milliseconds  milli( int n_ms);

        class IProcess
        {

        public:

            virtual ~IProcess() = default;
            /**
             *
             * execute process based on input parameters.
             * @param path, location of application to execute, will also be used as the first argument.
             * @param arguments, zero or more arguments that will be passed to the executable
             * @param extraEnvironment, zero or more environment variables required by the executable
             * @pre Require extraEnvironment to have a non empty key.
             */
            virtual void exec( const std::string & path, const std::vector<std::string> & arguments,
                                const EnvPairVector& extraEnvironment) = 0;
            virtual void exec( const std::string & path, const std::vector<std::string> & arguments) = 0;

            /**
            *
            * wait for process to complete or timeout
            * @param period - time lapsed for each attempt
            * @param attempt - number of times to run (period * attempts = total time to wait)
            * @return ProcessStatus
            */
            virtual ProcessStatus wait(Milliseconds period, int attempts ) = 0;

            /**
             *
             * Kill / terminate child process.
             * Returns False if the SIGTERM was enough
             *
             * @return true if we have to SIGKILL the process.
             *
            */
            virtual bool kill() = 0;

            /**
             *
             * exitCode of the child process, return only when child process has completed. If child process has not finished call will be blocked.
             * @return errorcode, 0 success, anything else failure.
             * @pre requires exec to be called before this method.
             */
            virtual int exitCode() = 0;

            /**
             *
             * output stdout and stderr from child process.
             *  Most recent limit bytes if setOutputLimit called.
             *
             * @return string, only when child process has completed
             * @pre requires exec to be called before this method.
             */
            virtual std::string output() = 0;

            /**
             * Non-blocking get the current status of the process
             * @pre requires exec to have been called before this method
             * @return ProcessStatus
             */
            virtual ProcessStatus getStatus() = 0;

            /**
             * Set a limit on how much output we keep for this process.
             * We'll keep the most recent limit bytes of output.
             *
             * @param limit
             */
            virtual void setOutputLimit(size_t limit) = 0;
        };
        using IProcessPtr = std::unique_ptr<IProcess>;
        extern IProcessPtr createProcess();
    }
}


#endif //COMMON_PROCESS_IPROCESS_H
