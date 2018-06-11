//
// Created by pair on 05/06/18.
//

#include <cassert>
#include <tuple>
#include "Product.h"
#include "SULUtils.h"

namespace SulDownloader
{
    //enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
    Product::Product(ProductInformation productInformation)
            : m_state(State::Initialized), m_error(), m_productInformation(productInformation), m_distributePath()
    {

    }

    bool Product::verify()
    {
        assert(m_state == State::Distributed);
        m_state = State::Verified;
        return false;
    }


    void Product::install()
    {
        assert( m_state == State::Verified);
        m_state = State::Installed;
    }

    bool Product::hasError() const
    {
        return !m_error.Description.empty();
    }

    void Product::setError(const std::string &error)
    {
        //assert(m_state != State::HasError);
        m_state = State::HasError;
        m_error.Description = error;
        std::tie(m_error.status, m_error.SulError) = getSulCodeAndDescription(SU_getSession(m_productInformation.getPHandle()));
    }

    WarehouseError Product::getError() const
    {
        return m_error;
    }

    std::string Product::distributionFolderName()
    {
        return m_productInformation.getName() + m_productInformation.getBaseVersion();
    }

    bool Product::setDistributePath(const std::string &distributePath)
    {
        m_state = State::Distributed;
        const char *empty = "";
        m_distributePath = distributePath;
        if ( !SULUtils::isSuccess(SU_addDistribution(m_productInformation.getPHandle(), m_distributePath.c_str(),
                           SU_AddDistributionFlag_UseDefaultHomeFolder, empty,
                           empty)))
        {
            setError( "Failed to set distribution path");

            return false;
        }
        return true;
    }

    void Product::verifyDistributionStatus()
    {
        assert( m_state == State::Distributed);
        if (! SULUtils::isSuccess(SU_getDistributionStatus(m_productInformation.getPHandle(), m_distributePath.c_str())))
        {
            SULUtils::displayLogs(SU_getSession(m_productInformation.getPHandle()));
            setError(std::string("Product distribution failed: ") + m_productInformation.getName()); //FIXME: get sul code to central
        }
    }


    ProductInformation Product::getProductInformation()
    {
        return m_productInformation;
    }

}