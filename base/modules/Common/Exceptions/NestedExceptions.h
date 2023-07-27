// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "IException.h"

namespace Common
{
    namespace Exceptions
    {
        namespace NestedExceptions
        {
            static std::string expandException(const std::exception& ex, int level);
            static std::string expandException(const IException& ex, int level);

            static std::string expandException(const IException& ex, int level = 0)
            {
                std::stringstream exceptionStringStream;
                exceptionStringStream << std::string(level, ' ')
                                      << "Nested exception expansion: " << ex.what_with_location() << '\n';

                try
                {
                    std::rethrow_if_nested(ex);
                }
                catch (const IException& nestedEx)
                {
                    exceptionStringStream << expandException(nestedEx, level + 1);
                }
                catch (const std::exception& nestedEx)
                {
                    exceptionStringStream << expandException(nestedEx, level + 1);
                }

                return exceptionStringStream.str();
            }

            static std::string expandException(const std::exception& ex, int level = 0)
            {
                std::stringstream exceptionStringStream;
                exceptionStringStream << std::string(level, ' ') << "Nested exception expansion: " << ex.what() << '\n';

                try
                {
                    std::rethrow_if_nested(ex);
                }
                catch (const IException& nestedEx)
                {
                    exceptionStringStream << expandException(nestedEx, level + 1);
                }
                catch (const std::exception& nestedEx)
                {
                    exceptionStringStream << expandException(nestedEx, level + 1);
                }

                return exceptionStringStream.str();
            }
        }
    }
}