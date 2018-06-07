//
// Created by pair on 05/06/18.
//

#include "Product.h"
namespace SulDownloader
{

    Product::Product(ProductInformation productInformation)
            : m_productInformation(productInformation), m_state(State::Initialized)
    {

    }

    bool Product::verify()
    {
        return false;
    }


    void Product::install()
    {

    }

    bool Product::hasError() const
    {
        return false;
    }

    void Product::setError(const std::string &error)
    {
        m_error = error;
    }

    const std::string &Product::getError() const
    {
        return m_error;
    }

    std::string Product::distributionFolderName()
    {
        return m_productInformation.getName() + m_productInformation.getBaseVersion();
    }

    void Product::setDistributePath(const std::string &distributePath)
    {
        const char *empty = "";
        m_distributePath = distributePath;
        SU_addDistribution(m_productInformation.getPHandle(), m_distributePath.c_str(),
                           SU_AddDistributionFlag_UseDefaultHomeFolder, empty,
                           empty);
    }

    SU_Result Product::getDistributionStatus()
    {
        return SU_getDistributionStatus(m_productInformation.getPHandle(), m_distributePath.c_str());
    }

}