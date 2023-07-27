// Copyright 2023 Sophos Limited. All rights reserved.

#include "IException.h"
#include <sstream>

namespace Common
{
    namespace Exceptions
    {
        namespace NestedExceptions
        {
            static std::string expandException(const std::exception& ex, int level);
            static std::string expandException(const IException& ex, int level);

            std::string rethrowIfPossible(const std::exception& ex, int level)
            {
                try
                {
                    std::rethrow_if_nested(ex);
                }
                catch (const IException& nestedEx)
                {
                    return expandException(nestedEx, level);
                }
                catch (const std::exception& nestedEx)
                {
                    return expandException(nestedEx, level);
                }

                return "";
            }

            static std::string expandException(const IException& ex, int level = 0)
            {
                std::stringstream exceptionStringStream;
                exceptionStringStream
                    << std::string(level, ' ')
                    << "Nested exception expansion: "
                    << ex.what()
                    << '\n';

                exceptionStringStream << rethrowIfPossible(ex, level + 1);

                return exceptionStringStream.str();
            }

            static std::string expandException(const std::exception& ex, int level = 0)
            {
                std::stringstream exceptionStringStream;
                exceptionStringStream
                    << std::string(level, ' ')
                    << "Nested exception expansion: "
                    << ex.what()
                    << '\n';

                exceptionStringStream << rethrowIfPossible(ex, level + 1);

                return exceptionStringStream.str();
            }
        }
    }
}
