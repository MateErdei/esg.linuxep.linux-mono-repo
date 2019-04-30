/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <vector>
#include <functional>

namespace Common::Telemetry
{
    class ITelemetryHelper
    {
    public:
        virtual ~ITelemetryHelper() = default;

        /// Set a ket to be an integer value
        /// \param key
        /// \param value
        virtual void set(const std::string& key, int value) = 0;

        /// Set a ket to be an unsigned integer value
        /// \param key
        /// \param value
        virtual void set(const std::string& key, unsigned int value) = 0;

        /// Set a ket to be a string
        /// \param key
        /// \param value
        virtual void set(const std::string& key, const std::string& value) = 0;

        /// Set a ket to be a string (covers C++ bad implicit casting of char* to bool)
        /// \param key
        /// \param value
        virtual void set(const std::string& key, const char* value) = 0;

        /// Set a ket to be a bool value
        /// \param key
        /// \param value
        virtual void set(const std::string& key, bool value) = 0;

        /// Increment the respective value of a key by an int
        /// Throws a std::logic_error if the value does not exist
        /// \param key
        /// \param value
        virtual void increment(const std::string& key, int value) = 0;

        /// Increment the respective value of a key by an unsigned int
        /// Throws a std::logic_error if the value does not exist
        /// \param key
        /// \param value
        virtual void increment(const std::string& key, unsigned int value) = 0;

        /// Append an int to an array, or if the array does not exist create it.
        /// \param key
        /// \param value
        virtual void append(const std::string& key, int value) = 0;

        /// Append an unsigned int to an array, or if the array does not exist create it.
        /// \param key
        /// \param value
        virtual void append(const std::string& key, unsigned int value) = 0;

        /// Append a string to an array, or if the array does not exist create it.
        /// \param key
        /// \param value
        virtual void append(const std::string& key, const std::string& value) = 0;

        /// Append a string to an array, or if the array does not exist create it.
        /// \param key
        /// \param value
        virtual void append(const std::string& key, const char* value) = 0;

        /// Append a string to an array, or if the array does not exist create it.
        /// \param key
        /// \param value
        virtual void append(const std::string& key, bool value) = 0;

        /// Register a callback function to be executed when the telemetry has been sent and cleared.
        /// \param cookie - A unique string to link the callback function to the callee
        /// \param function
        virtual void registerResetCallback(std::string cookie, std::function<void()> function) = 0;

        /// Remove a callback function using the same unique cookie that was used to register it by the callee.
        /// \param cookie
        virtual void unregisterResetCallback(std::string cookie) = 0;

        /// Clear all data and call all reset callback functions that have been registered.
        virtual void reset() = 0;

    };
}