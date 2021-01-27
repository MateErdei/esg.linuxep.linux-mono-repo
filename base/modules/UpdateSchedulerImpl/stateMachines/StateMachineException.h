/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <exception>
#include <string>
namespace UpdateScheduler
{
    namespace StateData
    {
        class StateMachineException : public std::exception
        {
        public:
            explicit StateMachineException(std::string message);

            const char* what() const noexcept override;

        private:
            std::string m_message;
        };

    } // namespace suldownloaderdata
} // namespace SulDownloader
