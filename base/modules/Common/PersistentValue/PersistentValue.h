// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFileSystem.h"

#include <iostream>
#include <sstream>
#include <string>

namespace Common
{
    /*
     * This is a header only class which can be used to persist single values to disk.
     * The values must work with the stream operators (<< and >>) e.g. int, string, float
     * Usage is simple, pass in a path to an existing var directory and the unique name of the value
     * that you wish to persist along with a default value which will be used if the persisting file
     * cannot be found.
     * This is templated, an example of storing an int:
     *     Common::PersistentValue<int> value("/opt/sophos-spl/base/var", "myValue", 0);
     * Then the following can be used:
     *     value.setValue(14);
     *     auto val = value.getValue(); // val = 14
     * On construction the value is auto loaded from disk
     * On Destruction the value is stored to disk so users of this class do not need to worry about writing the
     * value to disk.
     */
    template<class T>
    class PersistentValue
    {
    public:
        PersistentValue(const std::string& pathToVarDir, const std::string& name, T defaultValue) :
            defaultValue_(defaultValue), pathToFile_(Common::FileSystem::join(pathToVarDir, "persist-" + name))
        {
            try
            {
                loadValue();
            }
            catch (const std::exception& exception)
            {
                // Could not load value from file
                value_ = defaultValue_;
                errorMessage_ = exception.what();
            }
        }

        ~PersistentValue()
        {
            try
            {
                storeValue();
            }
            catch (std::exception& exception)
            {
                // Not a lot we can do if this happens
                // Can't use log4cplus since we want to use this header without logging setup
                std::cerr << "ERROR Failed to save value to " << pathToFile_ << " with error " << exception.what();
            }
        }

        void setValue(const T value) { value_ = value; }

        T getValue() const { return value_; }

        void setValueAndForceStore(const T value)
        {
            setValue(value);
            storeValue();
        }

        std::string getError() { return errorMessage_; }

    private:
        T value_;
        T defaultValue_;
        std::string pathToFile_;
        std::string errorMessage_;

        void loadValue()
        {
            auto fs = Common::FileSystem::fileSystem();
            if (fs->exists(pathToFile_))
            {
                auto valueString = fs->readFile(pathToFile_);
                std::stringstream valueStringStream(valueString);
                T value;
                valueStringStream >> value;
                value_ = value;
            }
            else
            {
                value_ = defaultValue_;
            }
        }

        void storeValue()
        {
            auto fs = Common::FileSystem::fileSystem();
            std::stringstream valueAsString;
            valueAsString << value_;
            fs->writeFile(pathToFile_, valueAsString.str());
        }
    };
} // namespace Common
