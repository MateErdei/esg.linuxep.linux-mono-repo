/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "AsyncMessager.h"
namespace CommsComponent
{
    class OtherSideApi
    {
        public:
        OtherSideApi(std::unique_ptr<AsyncMessager> messager); 
        ~OtherSideApi(); 
        void pushMessage(std::string );
        void close(); 
        void setNofifyOnClosure(NofifySocketClosed ); 
        private:
        std::unique_ptr<AsyncMessager> m_messager; 
    };
}