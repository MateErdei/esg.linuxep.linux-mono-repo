//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_PRODUCTS_H
#define SULDOWNLOADER_PRODUCTS_H

#include "ProductInformation.h"
#include "SULUtils.h"
#include "WarehouseError.h"
namespace SulDownloader
{
    class Product
    {
    public:
        explicit Product(  ProductInformation  );
        bool verify();

        void install();
        bool hasError() const;
        void setError( const std::string & );
        WarehouseError getError() const;
        std::string distributionFolderName();
        bool setDistributePath(const std::string & distributePath);
        void verifyDistributionStatus();
        ProductInformation getProductInformation();

    private :
        enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
        WarehouseError m_error;
        ProductInformation m_productInformation;
        std::string m_distributePath;
    };
}

#endif //SULDOWNLOADER_PRODUCTS_H
