//
// Created by pair on 12/06/18.
//


#ifndef EVEREST_BASE_COMMON_PROCESS_IPROCESS_H
#define EVEREST_BASE_COMMON_PROCESS_IPROCESS_H
#include <chrono>
#include <string>
#include <memory>
#include <vector>
namespace Common
{
    namespace Process
    {
        using Milliseconds = std::chrono::milliseconds;
        using EnvironmentPair = std::pair<std::string,std::string>;
        enum class ProcessStatus {FINISHED, TIMEOUT};
        Milliseconds  milli( int n_ms);

        class IProcess
        {

        public:

            virtual ~IProcess() = default;
            virtual void exec( const std::string & path, const std::vector<std::string> & arguments,
                                const std::vector<EnvironmentPair> & extraEnvironment={}) = 0;
            virtual ProcessStatus wait(Milliseconds period, int attempts ) = 0;
            virtual void kill() = 0;

            virtual int exitCode() = 0;

            virtual std::string output() = 0;
        };
        using IProcessPtr = std::unique_ptr<IProcess>;
        extern IProcessPtr createProcess();
    }
}


#endif //EVEREST_BASE_COMMON_PROCESS_IPROCESS_H
