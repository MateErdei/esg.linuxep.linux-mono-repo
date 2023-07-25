// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Location.h"

#include <exception>
#include <sstream>
#include <stdexcept>



namespace Common
{
    namespace Exceptions
    {
        class IException : public std::runtime_error
        {
        public:
            ~IException() override = default;
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

            [[nodiscard]] std::string what_with_location() const
            {
                if (file_)
                {
                    std::ostringstream ost;
                    ost << what() << ' ' << file_ << ':' << line_;
                    return ost.str();
                }
                return what();
            }
        protected:
            const char* file_;
            const unsigned long line_;
        };

        static std::string expandNestedException(const std::exception& ex, int level = 0)
        {
            std::string exceptionString{std::string(level, ' ') + "Exception: "};
            exceptionString += ex.what();
            exceptionString += '\n';

            try
            {
                std::rethrow_if_nested(ex);
            }
            catch (const std::exception& nestedEx)
            {
                exceptionString += expandNestedException(nestedEx, level + 1);
            }
            catch (...)
            {}

            return exceptionString;
        }
    } // namespace Exceptions
} // namespace Common