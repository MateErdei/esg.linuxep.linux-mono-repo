/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <iostream>
#include <stdexcept>
#include <typeinfo>

#define MAIN(TARGET)                                                                                                   \
    int main(int argc, char* argv[], char* envp[])                                                                     \
    {                                                                                                                  \
        static_cast<void>(argc);                                                                                       \
        static_cast<void>(argv);                                                                                       \
        static_cast<void>(envp);                                                                                       \
        try                                                                                                            \
        {                                                                                                              \
            return TARGET;                                                                                             \
        }                                                                                                              \
        catch (std::bad_alloc & e)                                                                                     \
        {                                                                                                              \
            std::cerr << "Unknown std::bad_alloc error, " << e.what() << std::endl;                                    \
            return 52;                                                                                                 \
        }                                                                                                              \
        catch (std::bad_cast & e)                                                                                      \
        {                                                                                                              \
            std::cerr << "Unknown std::bad_case error, " << e.what() << std::endl;                                     \
            return 53;                                                                                                 \
        }                                                                                                              \
        catch (std::bad_typeid & e)                                                                                    \
        {                                                                                                              \
            std::cerr << "Unknown std::bad_typeid error, " << e.what() << std::endl;                                   \
            return 54;                                                                                                 \
        }                                                                                                              \
        catch (std::bad_exception & e)                                                                                 \
        {                                                                                                              \
            std::cerr << "Unknown std::bad_exception error, " << e.what() << std::endl;                                \
            return 55;                                                                                                 \
        }                                                                                                              \
        catch (std::out_of_range & e)                                                                                  \
        {                                                                                                              \
            std::cerr << "Unknown std::out_of_range error, " << e.what() << std::endl;                                 \
            return 56;                                                                                                 \
        }                                                                                                              \
        catch (std::invalid_argument & e)                                                                              \
        {                                                                                                              \
            std::cerr << "Unknown std::invalid_argument error, " << e.what() << std::endl;                             \
            return 57;                                                                                                 \
        }                                                                                                              \
        catch (std::overflow_error & e)                                                                                \
        {                                                                                                              \
            std::cerr << "Unknown std::overflow_error error, " << e.what() << std::endl;                               \
            return 58;                                                                                                 \
        }                                                                                                              \
        catch (std::ios_base::failure & e)                                                                             \
        {                                                                                                              \
            std::cerr << "Unknown std::ios_base::failure error, " << e.what() << std::endl;                            \
            return 59;                                                                                                 \
        }                                                                                                              \
        catch (std::exception & e)                                                                                     \
        {                                                                                                              \
            std::cerr << "Unknown std::exception error, " << e.what() << std::endl;                                    \
            return 60;                                                                                                 \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            std::cerr << "Unknown error failed to obtain system data from diagnose." << std::endl;                     \
            return 61;                                                                                                 \
        }                                                                                                              \
        return 70;                                                                                                     \
    }
