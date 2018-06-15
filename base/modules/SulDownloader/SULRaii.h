/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_SULRAII_H
#define EVEREST_SULRAII_H
extern "C" {
#include <SUL.h>
}
#include <memory>

namespace SulDownloader{
    class SULSession
    {
    public:
        SULSession()
        {
            m_session = SU_beginSession();
        }

        ~SULSession()
        {
            if (m_session != nullptr)
            {
                SU_endSession(m_session);
            }
        }

        SU_Handle m_session = nullptr;
    };

    class SULInit
    {
    public:
        SULInit()
        {
            SU_init();
        }

        ~SULInit()
        {
            SU_deinit();
        }
    };



}


#endif //EVEREST_SULRAII_H
