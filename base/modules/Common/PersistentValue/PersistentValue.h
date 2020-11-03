/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystem/IFileSystem.h>

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
        template <class T>
        class PersistentValue
        {

        public:
            PersistentValue(std::string pathToVarDir, std::string name, T defaultValue)
                :
                m_defaultValue(defaultValue),
                m_pathToFile(Common::FileSystem::join(pathToVarDir, "persist-" + name))
            {
                try
                {
                    loadValue();
                }
                catch (std::exception& exception)
                {
//                  LOGERROR_PERSISTVALUE("Could not load value from: " << m_pathToFile);
                    m_value = m_defaultValue;
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
//                  LOGERROR_PERSISTVALUE("Could not persist value: " << m_pathToFile);
                }
            }

            void setValue(const T value)
            {
                m_value = value;
            }

            T getValue()
            {
                return m_value;
            }

        private:
            T m_value;
            T m_defaultValue;
            std::string m_pathToFile;

            void loadValue()
            {
                auto fs = Common::FileSystem::fileSystem();
                if (fs->exists(m_pathToFile))
                {
                    auto valueString = fs->readFile(m_pathToFile);
                    std::stringstream valueStringStream(valueString);
                    T value;
                    valueStringStream >> value;
                    m_value = value;
                }
                else
                {
                    m_value = m_defaultValue;
                }
            }

            void storeValue()
            {
                auto fs = Common::FileSystem::fileSystem();
                std::stringstream valueAsString;
                valueAsString << m_value;
                fs->writeFile(m_pathToFile, valueAsString.str());
            }
        };
} // namespace Common
