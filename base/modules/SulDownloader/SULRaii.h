//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_SULRAII_H
#define EVEREST_SULRAII_H
extern "C" {
#include <SUL.h>
}
#include <memory>
#include <iostream>
namespace SulDownloader{
    class SULSession
    {
    public:
        SULSession()
        {
            std::cerr << "create su_session\n";
            m_session = SU_beginSession();
        }

        ~SULSession()
        {
            std::cerr << "remove su_session\n";
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
