// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Location.h"

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace Exceptions
    {
        class IException : public std::runtime_error
        {
        public:
            virtual ~IException() = default;
            explicit IException(const std::string& what)
                :
                std::runtime_error(what),
                file_(nullptr),
                line_(0)
            {}

            IException(const char* file, const unsigned long line, const std::string& what) :
                std::runtime_error(what),
                file_(file),
                line_(line)
            {}

            std::string what_with_location() const
            {
                if (file_)
                {
                    return what() + std::string(file_) + ':' + std::to_string(line_);
                }
                else
                {
                    return what();
                }
            }
        protected:
            const char* file_;
            const unsigned long line_;
        };
    } // namespace Exceptions
} // namespace Common
