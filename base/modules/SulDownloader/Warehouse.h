/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_WAREHOUSE_H
#define EVEREST_WAREHOUSE_H
extern "C" {
#include <SUL.h>
}
#include "ConnectionSelector.h"
#include "ConfigurationData.h"
#include "Tag.h"
#include "WarehouseError.h"
#include <memory>

namespace SulDownloader
{
    class Product;
    class ProductSelection;
    class SULSession;

    class Warehouse
    {
    public:
        /**
         * Using the information in the configuration data object, a remote warehouse connection will be attempted
         * Upon a successful connection, a pointer to that warehouse will be returned.
         * @param configurationData containing required parameters for SUL to perform warehouse download
         * @return pointer to successfully connected warehouse
         */
        static std::unique_ptr<Warehouse> FetchConnectedWarehouse( const ConfigurationData & configurationData );

        Warehouse() = delete;
        Warehouse( const Warehouse & ) = delete;
        Warehouse &operator = (const Warehouse & ) = delete;

        Warehouse& operator = (Warehouse && ) = default;
        Warehouse( Warehouse&& ) = default;
        ~Warehouse();

        /**
         * Used to check if the warehouse reported an error
         * @return true, if warehouse has error, false otherwise
         */
        bool hasError() const;

        /**
         * Gets the warehouse error information.
         * @return struct containing the error description, sul error and status
         */
        WarehouseError getError() const;

        /**
         * Will download a given product file changes from remote warehouse repository to the local warehouse repository.
         * @param productSelection, provides the details for the selected product available to be downloaded / synchronised
         */
        void synchronize( ProductSelection & productSelection);

        /**
         * Will extract the file content from the local warehouse repository and create real product file structure required.
         *
         */
        void distribute();

        /**
         * Gets the list of products available in the warehouse
         * @return list of products
         */
        std::vector<Product> getProducts() const;

    private:
        enum class State{ Initialized, Failure, Synchronized, Connected, Distributed} m_state;

        void setError( const std::string & );
        void setConnectionSetup( const ConnectionSetup & connectionSetup, const ConfigurationData & configurationData);
        int  logLevel( ConfigurationData::LogLevel );
        explicit  Warehouse( bool createSession  );

        void distributeProduct( std::pair<SU_PHandle, Product> & productPair, const  std::string & distributePath );
        void verifyDistributeProduct( std::pair<SU_PHandle, Product> & productPair);

        WarehouseError fetchSulError(const std::string & description ) const;

        SU_Handle session() const;
        WarehouseError m_error;
        std::vector<std::pair<SU_PHandle, Product>> m_products;
        std::unique_ptr<SULSession> m_session ;
        std::unique_ptr<ConnectionSetup> m_connectionSetup;
    };
}



#endif //EVEREST_WAREHOUSE_H
