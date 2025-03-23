
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "nlohmann/json_fwd.hpp"

#include "Item.hpp"
#include "FSConfig.hpp"
#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"
#include "andromeda/backend/RunnerInput.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class Folder;

namespace Filedata { class PageManager; class PageBackend; }

/** 
 * An Andromeda file that can be read/written
 * THREADING - get locks in advance and pass to functions
 */
class File : public Item
{
public:

    /** Exception indicating that the read goes out of bounds */
    class ReadBoundsException : public Exception { public:
        ReadBoundsException() : Exception("Read Out of Range") {}; };

    /** Exception indicating that the filesystem does not support writing */
    class WriteTypeException : public Exception { public:
        WriteTypeException() : Exception("Write Type Unsupported") {}; };

    using ScopeLocked = Andromeda::ScopeLocked<File>;
    /** Tries to lock mScopeMutex, returns a ref that is maybe locked */
    ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    ~File() override;
    DELETE_COPY(File)
    DELETE_MOVE(File)

    void Refresh(const nlohmann::json& data, const SharedLockW& thisLock) override;

    Type GetType() const override { return Type::FILE; }

    /** Returns the total file size */
    virtual uint64_t GetSize(const SharedLock& thisLock) const final;

    /** Returns the file's data page size */
    virtual size_t GetPageSize() const;

    /** Checks the storage and account policy for the allowed write mode */
    FSConfig::WriteMode GetWriteMode() const;

    /**
     * @brief Construct a File using backend data
     * @param backend backend reference
     * @param data JSON data from backend
     * @param parent reference to parent folder
     * @throws BackendException on backend errors
     */
    File(Backend::BackendImpl& backend, const nlohmann::json& data, Folder& parent);

    /** Function to create the file on the backend and return its JSON */
    using CreateFunc = std::function<nlohmann::json (const std::string&)>;
    /** Function to upload the file on the backend and return its JSON */
    using UploadFunc = std::function<nlohmann::json (const std::string&, const Andromeda::Backend::WriteFunc&, bool)>;

    /**
     * @brief Construct a new file in memory only to be created on the backend when flushed
     * @param backend backend reference
     * @param parent reference to parent folder
     * @param name name of the new file
     * @param stConfig reference to storage config
     * @param createFunc function to create on the backend
     * @param uploadFunc function to upload on the backend
     */
    File(Backend::BackendImpl& backend, Folder& parent, 
        const std::string& name, const FSConfig& stConfig,
        const CreateFunc& createFunc, const UploadFunc& uploadFunc);

    /** Returns true iff the file exists on the backend (false if waiting for flush) */
    virtual bool ExistsOnBackend(const SharedLock& thisLock) const;

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length max number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    virtual size_t ReadBytesMax(char* buffer, uint64_t offset, size_t maxLength, const SharedLock& thisLock) final;

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length exact number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     * @throws ReadBoundsException if invalid offset/length
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    virtual void ReadBytes(char* buffer, uint64_t offset, size_t length, const SharedLock& thisLock) final;

    /**
     * Writes data to a file
     * @param buffer buffer with data to write
     * @param offset byte offset in file to write
     * @param length number of bytes to write
     * @throws WriteTypeException if ExistsOnBackend() and writing UPLOAD only files or non-appending write to APPEND only files 
     * @throws ReadOnlyFSException if read-only item/filesystem
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    virtual void WriteBytes(const char* buffer, uint64_t offset, size_t length, const SharedLockW& thisLock) final;

    /** 
     * Set the file size to the given value
     * @throws WriteTypeException if write mode is UPLOAD, or write mode is APPEND and newSize != 0 
     * @throws ReadOnlyFSException if read-only item/filesystem
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    virtual void Truncate(uint64_t newSize, const SharedLockW& thisLock) final;

    void FlushCache(const SharedLockW& thisLock, bool nothrow = false) override;

protected:

    void SubDelete(const DeleteLock& deleteLock) override;

    void SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite) override;

    void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite) override;

private:

    /** Returns the page size calculated from the backend.pageSize and stConfig.chunkSize */
    size_t CalcPageSize() const;

    /**
     * Writes to the backend until it aligns with a page boundary or the buffer runs out
     * @param buffer data buffer to consume as needed
     * @param offset byte offset of the data buffer
     * @param length number of bytes in the buffer
     * @return size_t number of bytes consumed from buffer
     * @throws BackendException for backend issues
     */
    size_t FixPageAlignment(const char* buffer, uint64_t offset, size_t length, const SharedLockW& thisLock);

    /** Calls WriteBytes() with zeroes until the file size equals offset */
    void FillWriteHole(uint64_t offset, const SharedLockW& thisLock);

    std::unique_ptr<Filedata::PageManager> mPageManager;
    std::unique_ptr<Filedata::PageBackend> mPageBackend;

    mutable Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FILE_H_
