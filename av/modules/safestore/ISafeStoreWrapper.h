// Copyright 2022, Sophos Limited.  All rights reserved.

/*
 * This is a C++ wrapper around the Core safestore C library.
 * This wrapper interface provides two main benefits, to be able to call the library via C++ methods and to allow
 * unit tests to be written by being able to create a mock of this interface.
 */

#pragma once

#include <string>

namespace safestore
{
    // Return codes from SafeStore initialisation
    enum InitReturnCode
    {
        // SafeStore initialised ok
        OK,
        // ...add comments to all  codes here:
        INVALID_ARG,
        UNSUPPORTED_OS,
        UNSUPPORTED_VERSION ,
        OUT_OF_MEMORY,
        DB_OPEN_FAILED,
        DB_ERROR,

        // Failed to initialise for unknown reason
        FAILED
    };

    enum SaveFileReturnCode
    {
        // SafeStore initialised ok
        OK,
        // ...add comments to all  codes here:
        INVALID_ARG,
        UNSUPPORTED_OS,
        UNSUPPORTED_VERSION ,
        OUT_OF_MEMORY,
        DB_OPEN_FAILED,
        DB_ERROR,

        // Failed to initialise for unknown reason
        FAILED
    };

//    class SafeStoreContext
//    {
//
//
//    private:
//        void* m_ctx;
//    };

    using SafeStoreContext = void*;
    using SafeStoreObjectHandle = void*;

    class ISafeStoreWrapper
    {
    public:
        virtual ~ISafeStoreWrapper() = default;

        /*
         * Initialise SafeStore and setup any resources needed.
         * Returns: InitReturnCode, to indicate if the operation was successful or not.
         */

//        _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_Init(
//            _Deref_out_ SafeStore_t* ctx,
//            _In_z_ const SsPlatChar* dbDirectory,
//            _In_z_ const SsPlatChar* dbFileName,
//            _In_count_(passwordSize) const uint8_t* password,
//            _In_ size_t passwordSize,
//            _In_ int flags);

        virtual InitReturnCode initialise(SafeStoreContext ctx, const std::string& dbDirName, const std::string& dbName, const std::string& password) = 0;

        virtual void deinitialise(SafeStoreContext ctx) = 0;

        ///* Saves a file to SafeStore.
        // *
        // * Parameters:
        // *   ctx (in) - initialized SafeStore context handle
        // *   directory (in) - absolute path of the directory where the file is located
        // *   fileName (in) - name of the file to store
        // *   threatId (in) - identifier of the threat the file is to be associated with
        // *   threatName (in) - name of the threat the file is to be associated with
        // *   objectHandle (out, optional) - SafeStore handle of the stored object
        // *
        // * Return value:
        // *   Success:
        // *     SR_OK
        // *   Failure:
        // *     SR_INVALID_ARG - an invalid argument was passed to the function
        // *     SR_INTERNAL_ERROR - an internal error has occurred
        // *     SR_OUT_OF_MEMORY - not enough memory is available to complete the operation
        // *     SR_FILE_OPEN_FAILED - failed to open the file (or the blob file)
        // *     SR_FILE_READ_FAILED - failed to read the file
        // *     SR_FILE_WRITE_FAILED - failed to write the blob file
        // *     SR_MAX_OBJECT_SIZE_EXCEEDED - size of the file is greater than the maximum
        // *       allowed object size (SC_MAX_OBJECT_SIZE config option)
        // *     SR_MAX_STORE_SIZE_EXCEEDED - saving the file would grow SafeStore above
        // *       the maximum allowed size (SC_MAX_SAFESTORE_SIZE config option)
        // *     SR_DB_ERROR - database operation failed
        // *
        // * Remarks:
        // *   The object gets the SOS_STORED status after it is saved.
        // *   Use SafeStore_ReleaseObjectHandle to release the object handle.
        // *   SafeStore stores the directory path it receives as argument in order to
        // *   later be able to restore the file to its original location (if needed).
        // *   The directory argument needs to be an absolute path so the file can always
        // *   be restored to the same location, even if the current directory changes
        // *   between the save and the restore operations.
        // *   A blob file in the context of SafeStore is a compressed and encrypted
        // *   file an object is stored in.
        // *   This function invokes database change operations that invalidate existing
        // *   object handles. The behaviour when using invalidated object handles is
        // *   undefined.
        // */
        //_Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_SaveFile(
        //    _In_ SafeStore_t ctx,
        //    _In_z_ const SsPlatChar* directory,
        //    _In_z_ const SsPlatChar* fileName,
        //    _In_ const SafeStore_Id_t* threatId,
        //    _In_z_ const SsPlatChar* threatName,
        //    _Deref_opt_out_ SafeStore_Handle_t* objectHandle);

        virtual void saveFile(SafeStoreContext ctx, std::string directory, std::string filename, std::string threatId, std::string threatName, SafeStoreObjectHandle objectHandle);

    };
}
