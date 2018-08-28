/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "IVersig.h"
#include <functional>
namespace SulDownloader
{
    class VersigImpl: public virtual IVersig
    {
    public:
        VerifySignature verify( const std::string & certificate_path, const std::string & productDirectoryPath ) const override ;
    };



    class VersigFactory
    {
        VersigFactory();
        std::function<std::unique_ptr<IVersig>(void)> m_creator;
    public:
        static VersigFactory & instance();
        std::unique_ptr<IVersig> createVersig();
        // for tests only
        void replaceCreator(std::function<std::unique_ptr<IVersig>(void)>creator);
        void restoreCreator();
    };
}


