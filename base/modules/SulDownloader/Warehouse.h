//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_WAREHOUSE_H
#define EVEREST_WAREHOUSE_H

#include "ConnectionSelector.h"
#include "ConfigurationData.h"
#include "SULUtils.h"
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
        Error getError() const;
        void synchronize( ProductSelection & );
        void distribute();
        std::vector<Product> & getProducts();
    private:
        enum class State{ Initialized, Failure, Synchronized, Connected, Distributed} m_state;

        void setError( const std::string & );
        void setConnectionSetup( const ConnectionSetup & connectionSetup);
        explicit  Warehouse( bool createSession  );

        SU_Handle session();
        Error m_error;
        std::vector<Product> m_products;
        std::unique_ptr<SULSession> m_session ;
        std::unique_ptr<ConnectionSetup> m_connectionSetup;
    };
}



#endif //EVEREST_WAREHOUSE_H
