/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//	This file is originally from Messaging System\CertificationClientLibrary project.
//	Modified to use as part of obscurity library.
//

#pragma once

#include <stdexcept>
#include <string>

namespace Common
{
    namespace ObfuscationImpl
    {

        class ObscurityException
                : public std::runtime_error
        {

        public:


            /**
             * Default constructor
             */
            ObscurityException()
                    : std::runtime_error("ObscurityException:")
            {
            }

            /**
             * Constructor that takes a message
             * @param message
             */
            ObscurityException(const char* message)
                    : std::runtime_error(message)
            {
            }


            /**
             * Destructor
             */
            ~ObscurityException() throw()
            {}
        };
    }
}
