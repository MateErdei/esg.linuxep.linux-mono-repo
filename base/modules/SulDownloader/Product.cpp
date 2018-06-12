//
// Created by pair on 05/06/18.
//

#include <cassert>
#include <tuple>
#include "Product.h"
#include "Logger.h"


namespace SulDownloader
{
    //enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
    Product::Product(ProductInformation productInformation)
            : m_state(State::Initialized), m_error(), m_productInformation(productInformation), m_distributePath(), m_productHasChanged(false)
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
        LOGINFO("Installing product: " << m_productInformation.getLine() << " version: " << m_productInformation.getVersion());
    }

    bool Product::hasError() const
    {
        return !m_error.Description.empty();
    }

//    void Product::setError(const std::string &error)
//    {
//        m_state = State::HasError;
//        m_error.Description = error;
//        m_error.status = UNSPECIFIED;
//        m_error.SulError = "";
//    }

    void Product::setError(WarehouseError error)
    {
        m_state = State::HasError;
        m_error = error;
    }


    WarehouseError Product::getError() const
    {
        return m_error;
    }

    std::string Product::distributionFolderName()
    {
        return m_productInformation.getLine() + m_productInformation.getVersion();
    }

    void Product::setDistributePath(const std::string &distributePath)
    {
        m_state = State::Distributed;
        m_distributePath = distributePath;
    }

    ProductInformation Product::getProductInformation()
    {
        return m_productInformation;
    }

    std::string Product::distributePath() const
    {
        return m_distributePath;
    }

    std::string Product::getLine() const
    {
        return m_productInformation.getLine();
    }

    std::string Product::getName() const
    {
        return m_productInformation.getName();
    }

    bool Product::productHasChanged() const
    {
        return m_productHasChanged;
    }

    void Product::setProductHasChanged(bool productHasChanged)
    {
        m_productHasChanged = productHasChanged;
    }


}