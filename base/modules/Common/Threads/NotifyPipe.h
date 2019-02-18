///////////////////////////////////////////////////////////
//
// Copyright (C) 2004-2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
// Originally from //dev/savlinux/trunk/ifaces/internal/Common/Threads/NotifyPipe.h
///////////////////////////////////////////////////////////

#if !defined(EA_27C1BE20_A00E_43cb_802A_29A3A22C6E3D__INCLUDED_)
#    define EA_27C1BE20_A00E_43cb_802A_29A3A22C6E3D__INCLUDED_

#    include <unistd.h>

namespace Common
{
    namespace Threads
    {
        /**
         * @author Douglas Leeder
         * @version 1.0
         * @created 29-Apr-2008 14:10:31
         */
        class NotifyPipe
        {
        public:
            /**
             * Create a new NotifyPipe.
             * Creates a pipe().
             */
            NotifyPipe() noexcept;
            virtual ~NotifyPipe() noexcept;

            /**
             * @return True once for each time notify() is called.
             */
            bool notified();
            /**
             * Write a byte to the pipe.
             */
            ssize_t notify();
            /**
             * Get the receiving side of the Pipe. For use in select calls etc.
             */
            int readFd();
            /**
             * Get the sender side of the pipe. For use in signal handlers where the entire
             * object isn't wanted.
             */
            int writeFd();

        private:
            /**
             * The receiving end of the pipe.
             */
            int m_readFd;
            /**
             * The sending end of the pipe.
             */
            int m_writeFd;
        };
    } // namespace Threads

} // namespace Common
#endif // !defined(EA_27C1BE20_A00E_43cb_802A_29A3A22C6E3D__INCLUDED_)
