// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "TerminateHandler.h"

#include <iostream>
#include <stdexcept>
#include <typeinfo>

#define MAIN(TARGET)                                                                                                   \
    int main(int argc, char* argv[], char* envp[])                                                                     \
    {                                                                                                                  \
        static_cast<void>(argc);                                                                                       \
        static_cast<void>(argv);                                                                                       \
        static_cast<void>(envp);                                                                                       \
        std::set_terminate(terminate_handler);                                                                         \
        try                                                                                                            \
        {                                                                                                              \
            return TARGET;                                                                                             \
        }                                                                                                              \
        catch (const Common::Exceptions::IException& ex)                                                               \
        {                                                                                                              \
            std::cerr << "IException thrown and not caught: " << ex.what_with_location() << '\n';                      \
            return 51;                                                                                                 \
        }                                                                                                              \
        catch (const std::bad_alloc& e)                                                                                \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::bad_alloc error, " << e.what() << '\n';                              \
            return 52;                                                                                                 \
        }                                                                                                              \
        catch (const std::bad_cast& e)                                                                                 \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::bad_case error, " << e.what() << std::endl;                          \
            return 53;                                                                                                 \
        }                                                                                                              \
        catch (const std::bad_typeid& e)                                                                               \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::bad_typeid error, " << e.what() << std::endl;                        \
            return 54;                                                                                                 \
        }                                                                                                              \
        catch (const std::bad_exception& e)                                                                            \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::bad_exception error, " << e.what() << std::endl;                     \
            return 55;                                                                                                 \
        }                                                                                                              \
        catch (const std::out_of_range& e)                                                                             \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::out_of_range error, " << e.what() << std::endl;                      \
            return 56;                                                                                                 \
        }                                                                                                              \
        catch (const std::invalid_argument& e)                                                                         \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::invalid_argument error, " << e.what() << std::endl;                  \
            return 57;                                                                                                 \
        }                                                                                                              \
        catch (const std::overflow_error& e)                                                                           \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::overflow_error error, " << e.what() << std::endl;                    \
            return 58;                                                                                                 \
        }                                                                                                              \
        catch (const std::ios_base::failure& e)                                                                        \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::ios_base::failure error, " << e.what() << std::endl;                 \
            return 59;                                                                                                 \
        }                                                                                                              \
        catch (const std::exception& e)                                                                                \
        {                                                                                                              \
            std::cerr << "Critical unhandled std::exception error, " << e.what() << std::endl;                         \
            return 60;                                                                                                 \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            std::cerr << "Critical unhandled error. No information available." << std::endl;                           \
            return 61;                                                                                                 \
        }                                                                                                              \
        return 70;                                                                                                     \
    }
