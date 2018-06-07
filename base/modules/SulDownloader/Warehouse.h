//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_WAREHOUSE_H
#define EVEREST_WAREHOUSE_H

#include "ConnectionSelector.h"
#include "ConfigurationData.h"
#include "SULRaii.h"
#include "Tag.h"
#include "Warehouse.h"

namespace SulDownloader
{
    class Product;
    class ProductSelection;

    class Warehouse
    {
    public:
        static std::unique_ptr<Warehouse> FetchConnectedWarehouse( const ConfigurationData & configurationData );
        Warehouse() = delete;
        Warehouse( const Warehouse & ) = delete;
        Warehouse &operator = (const Warehouse & ) = delete;

        Warehouse& operator = (Warehouse && ) = default;
        Warehouse( Warehouse&& ) = default;
        ~Warehouse();

        bool hasError() const;
        const std::string& getError() const;
        void synchronize( ProductSelection & );
        void distribute();
        std::vector<Product> & getProducts();


    private:
        enum class State{ Initialized, ConnectionFailure, SyncFailure, Synchronized, Connected} m_state;

        void setError( const std::string & );
        void setConnectionSetup( const ConnectionSetup & connectionSetup);
        explicit  Warehouse( bool createSession  );

        SU_Handle session();
        std::string m_error;
        std::vector<Product> m_products;
        std::unique_ptr<SULSession> m_session ;
    };
}



#endif //EVEREST_WAREHOUSE_H
