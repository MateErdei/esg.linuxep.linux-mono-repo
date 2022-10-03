//// Copyright 2022, Sophos Limited.  All rights reserved.
//
//#include "SafeStoreRunner.h"
//
//#include "common/ApplicationPaths.h"
//
//namespace safestore
//{
//    SafeStoreRunner::SafeStoreRunner(std::unique_ptr<ISafeStoreWrapper> quarantineManager) : m_quarantineManager(std::move(quarantineManager)){}
//
//    void SafeStoreRunner::start()
//    {
//        // Note: verify how C will handle exceptions
//        auto result = m_quarantineManager->initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName());
//
//        switch(result)
//        {
//            case OK:
//                return;
//            case INVALID_ARG:
//                break;
//            case UNSUPPORTED_OS:
//                break;
//            case UNSUPPORTED_VERSION:
//                break;
//            case OUT_OF_MEMORY:
//                break;
//            case DB_OPEN_FAILED:
//                break;
//            case DB_ERROR:
//                break;
//            case FAILED:
//                break;
//        }
//    }
//}