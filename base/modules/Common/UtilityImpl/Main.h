/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <stdexcept>
#include <iostream>

#define MAIN(TARGET)                                                                                \
int main(int argc, char* argv[], char* envp[])                                                      \
{                                                                                                   \
    static_cast<void>(argc);                                                                        \
    static_cast<void>(argv);                                                                        \
    static_cast<void>(envp);                                                                        \
    try                                                                                             \
    {                                                                                               \
        return TARGET;                                                                              \
    }                                                                                               \
    catch( std::bad_alloc& e)                                                                       \
    {                                                                                               \
        std::cerr << "Uncaught std::bad_alloc " << e.what() << std::endl;                           \
        return 52;                                                                                  \
    }                                                                                               \
    catch( std::bad_cast& e)                                                                        \
    {                                                                                               \
        std::cerr << "Uncaught std::bad_case " << e.what() << std::endl;                            \
        return 53;                                                                                  \
    }                                                                                               \
    catch( std::bad_typeid& e)                                                                      \
    {                                                                                               \
        std::cerr << "Uncaught std::bad_typeid " << e.what() << std::endl;                          \
        return 54;                                                                                  \
    }                                                                                               \
    catch( std::bad_exception& e)                                                                   \
    {                                                                                               \
        std::cerr << "Uncaught std::bad_exception " << e.what() << std::endl;                       \
        return 55;                                                                                  \
    }                                                                                               \
    catch( std::out_of_range& e)                                                                    \
    {                                                                                               \
        std::cerr << "Uncaught std::out_of_range " << e.what() << std::endl;                        \
        return 56;                                                                                  \
    }                                                                                               \
    catch( std::invalid_argument& e)                                                                \
    {                                                                                               \
        std::cerr << "Uncaught std::invalid_argument " << e.what() << std::endl;                    \
        return 57;                                                                                  \
    }                                                                                               \
    catch( std::overflow_error& e)                                                                  \
    {                                                                                               \
        std::cerr << "Uncaught std::overflow_error " << e.what() << std::endl;                      \
        return 58;                                                                                  \
    }                                                                                               \
    catch( std::ios_base::failure& e)                                                               \
    {                                                                                               \
        std::cerr << "Uncaught std::ios_base::failure " << e.what() << std::endl;                   \
        return 59;                                                                                  \
    }                                                                                               \
    catch( std::exception& e)                                                                       \
    {                                                                                               \
        std::cerr << "Uncaught std::exception " << e.what() << std::endl;                           \
        return 60;                                                                                  \
    }                                                                                               \
    catch(...)                                                                                      \
    {                                                                                               \
        std::cerr << "Caught ... at top level." << std::endl;                                       \
        return 61;                                                                                  \
    }                                                                                               \
    return 70;                                                                                      \
}
