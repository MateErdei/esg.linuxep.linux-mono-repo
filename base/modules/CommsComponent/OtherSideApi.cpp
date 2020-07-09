/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OtherSideApi.h"

namespace CommsComponent {
    OtherSideApi::OtherSideApi(std::unique_ptr<AsyncMessager> messager):
        m_messager(std::move(messager))
        {
    }
    
    OtherSideApi::~OtherSideApi()
    {
        m_messager->setNotifyClosure([](){}); 
    }

    void OtherSideApi::setNofifyOnClosure(NofifySocketClosed notifyOnClosure)
    {
        m_messager->setNotifyClosure(notifyOnClosure);  
    }

    void OtherSideApi::pushMessage(std::string message){
        m_messager->sendMessage(std::move(message)); 
    }

    void OtherSideApi::close()
    {
        m_messager->setNotifyClosure([](){}); 
        m_messager->push_stop(); 
    }

} 