//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_PRODUCTS_H
#define SULDOWNLOADER_PRODUCTS_H

#include "ProductInformation.h"
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
        //void setError( const std::string & );
        void setError( WarehouseError error);
        WarehouseError getError() const;
        std::string distributionFolderName();
        void setDistributePath(const std::string & distributePath);
        std::string distributePath() const;
        ProductInformation getProductInformation();
        std::string getLine() const;
        std::string getName() const;
        bool productHasChanged() const;
        void setProductHasChanged( bool  );
    private :
        enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
        WarehouseError m_error;
        ProductInformation m_productInformation;
        std::string m_distributePath;
        bool m_productHasChanged;
    };
}

#endif //SULDOWNLOADER_PRODUCTS_H
