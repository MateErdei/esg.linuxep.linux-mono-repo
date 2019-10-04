/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <functional>
#include <memory>
#include <string>
namespace Common
{
    namespace UtilityImpl
    {
        /** Use this to create factory when mock is necessary for test purpose.
         * Look at how it is used in WatchdogServiceLine
         *
         * In a header where the interface is defined add:
         namespace XXX{
            Common::UtilityImpl::Factory<Interface>& factory();
         }
         * In a cpp file implements:
         *
         namespace XXX{
             Common::UtilityImpl::Factory<Interface>& factory()
             {
                static Common::UtilityImpl::Factory<Interface> theFactory{
                    [](){
                        return std::unique_ptr<Interface>(new Implementation());
                    }};
                return theFactory;
             }
         }
         * Use it as:
         *   auto object = XXX.factory().create();
         * In tests you may also use:
         *   XXX.factory().replace(creator);
         *   XXX.factory().restore();
         * You may also consider using a scoped variable to do this as in MockIWatchdogRequest.H
         * @tparam InterfaceToCreate
         */
        template<typename InterfaceToCreate>
        class Factory
        {
            std::function<std::unique_ptr<InterfaceToCreate>()> m_original;
            std::function<std::unique_ptr<InterfaceToCreate>()> m_functor;

        public:
            Factory(std::function<std::unique_ptr<InterfaceToCreate>()> fun) : m_original(fun), m_functor(fun) {}
            std::unique_ptr<InterfaceToCreate> create() { return m_functor(); }
            void replace(std::function<std::unique_ptr<InterfaceToCreate>()> fun) { m_functor = fun; }
            void restore() { m_functor = m_original; }
        };
    } // namespace UtilityImpl
} // namespace Common
