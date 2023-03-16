
#ifndef LIBA2_PAGEMANAGER_H_
#define LIBA2_PAGEMANAGER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <list>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "BandwidthMeasure.hpp"
#include "PageBackend.hpp"
#include "Page.hpp"

#include "andromeda/Debug.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class File;

namespace Filedata {

class CacheManager;

/** 
 * File page data manager - splits the file into a series of fixed size pages
 * Implements thread-safe interfaces to read, write, truncate, evict and flush
 * Implements various tricks/caching to greatly increase speed:
 *  - caches pages read from the backend (see EvictPage)
 *  - reads ahead consecutive ranges of pages sized by bandwidth,
 *      doing so on a background thread to minimize waiting
 *  - caches writes until flushed (write-back cache) (see FlushPage)
 *  - writes back consecutive ranges of pages to maximize throughput
 *  - supports delayed file Create to combine Create+Write to Upload
 */
class PageManager
{
public:

    /** 
     * Construct a new file page manager
     * @param file reference to the parent file
     * @param fileID reference to the file's backend ID
     * @param fileSize initial file size
     * @param pageSize size of pages to use (const)
     * @param pageBackend page backend reference
     */
    PageManager(File& file, const uint64_t fileSize, const size_t pageSize, PageBackend& pageBackend);

    virtual ~PageManager();

    /** Returns the page size in use */
    size_t GetPageSize() const { return mPageSize; }

    /** Returns the current file size including dirty writes */
    uint64_t GetFileSize() const { return mFileSize; }

    /** Returns the current size on the backend (we may have dirty writes) */
    uint64_t GetBackendSize() const { return mPageBackend.GetBackendSize(); }

    /** Returns whether or not the file exists on the backend */
    bool ExistsOnBackend() const { return mPageBackend.ExistsOnBackend(); }

    /** Returns a read lock for page data */
    SharedLockR GetReadLock() { return SharedLockR(mDataMutex); }
    
    /** Returns a priority read lock for page data */
    SharedLockRP GetReadPriLock() { return SharedLockRP(mDataMutex); }
    
    /** Returns a write lock for page data */
    SharedLockW GetWriteLock() { return SharedLockW(mDataMutex); }

    typedef std::shared_lock<std::shared_mutex> ScopeLock;

    /** Tries to lock mScopeMutex, returns a lock object that is maybe locked */
    ScopeLock TryGetScopeLock() { return ScopeLock(mScopeMutex, std::try_to_lock); }

    /** Reads data from the given page index into buffer */
    void ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock);

    /** Writes data to the given page index from buffer */
    void WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock);

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index, const SharedLockW& dataLock);

    /** Removes the given page, writing it if dirty */
    void EvictPage(const uint64_t index, const SharedLockW& dataLock);

    /** 
     * Flushes the given page if dirty
     * Will also flush any dirty pages sequentially after this one
     * @return the total number of bytes written to the backend
     */
    size_t FlushPage(const uint64_t index, const SharedLockRP& dataLock);
    size_t FlushPage(const uint64_t index, const SharedLockW& dataLock);

    /** Writes back all dirty pages - THREAD SAFE */
    void FlushPages();

    /**
     * Informs us of the file changing on the backend - THREAD SAFE
     * @param backendSize new size according to the backend
     */
    void RemoteChanged(const uint64_t backendSize);

    /** Truncate pages according to the given size and inform the backend - THREAD SAFE */
    void Truncate(const uint64_t newSize);

private:

    typedef std::unique_lock<std::mutex> UniqueLock;

    /** Returns the page at the given index and informs cacheMgr - use GetReadLock() first! */
    const Page& GetPageRead(const uint64_t index, const SharedLockR& dataLock);

    /** 
     * Returns the page at the given index and marks dirty/informs cacheMgr - use GetWriteLock() first! 
     * @param pageSize the desired size of the page for writing
     * @param partial if true, pre-populate the page with backend data
     */
    Page& GetPageWrite(const uint64_t index, const size_t pageSize, const bool partial, const SharedLockW& dataLock);

    /** 
     * Calls mCacheMgr->InformPage() on the given page and removes it from mPages if it fails 
     * MUST HAVE either dataLockW or pagesLock! (will be checked!)
     */
    void InformNewPage(const uint64_t index, Page& page, bool dirty, bool canWait, const UniqueLock* pagesLock, const SharedLockW* dataLock);

    /** Resizes then calls mCacheMgr->InformPage() on the given page and restores size if it fails */
    void InformResizePage(const uint64_t index, Page& page, bool dirty, const size_t pageSize, const SharedLockW& dataLock);

    /** 
     * Resizes an existing page to the given size, telling the CacheManager if cacheMgr
     * CALLER MUST LOCK the DataLockW if operating on an existing page!
     */
    void ResizePage(Page& page, const size_t pageSize, bool cacheMgr, const SharedLockW* dataLock = nullptr);

    /** Returns true if the page at the given index is pending download */
    bool isFetchPending(const uint64_t index, const UniqueLock& pagesLock);

    /** Returns an exception_ptr if the page at the given index failed download else nullptr */
    std::exception_ptr isFetchFailed(const uint64_t index, const UniqueLock& pagesLock);

    /** 
     * Returns the read-ahead size to be used for the given VALID (mFileSize) index 
     * Returns 0 if the page exists or the index does not exist on the backend (see mBackendSize)
     */
    size_t GetFetchSize(const uint64_t index, const UniqueLock& pagesLock);

    /** Starts a fetch if necessary to prepopulate some pages ahead of the given index (options.readAheadBuffer) */
    void DoAdvanceRead(const uint64_t index, const UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages starting at the given VALID (mBackendSize) index */
    void StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock);

    /** 
     * Reads count# pages from the backend at the given index, adding to the page map
     * Gets its own R dataLock and informs the cacheManager of all new pages
     */
    void FetchPages(const uint64_t index, const size_t count);

    /** 
     * Removes the given start index from the pending-read list and notifies waiters
     * @param idxOnly if true, adjust the entry start index to be the next index, else remove it
     */
    void RemovePendingFetch(const uint64_t index, bool idxOnly, const UniqueLock& pagesLock);

    /** Updates mFetchSize with the given bandwidth measurement - THREAD SAFE */
    void UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time);

    /** Map of page index to page */
    typedef std::map<uint64_t, Page> PageMap;

    /** 
     * Returns a series of **consecutive** dirty pages (total bytes < size_t)
     * @param[in,out] pageIt reference to the iterator to start with - will end as the next index not used
     * @param[out] writeList reference to a list of pages to fill out - guaranteed not empty if pageIt is dirty
     * @return uint64_t the start index of the write list (not valid if writeList is empty!)
     */
    uint64_t GetWriteList(PageMap::iterator& pageIt, PageBackend::PagePtrList& writeList, const UniqueLock& pagesLock);

    /** Flushes the given page if dirty (already have the lock) - MUST HAVE DATALOCKR/W! */
    size_t FlushPage(const uint64_t index, const UniqueLock& flushLock);

    /** 
     * Writes a series of **consecutive** pages (total < size_t) - MUST HAVE DATALOCKR/W!
     * Also marks each page not dirty and informs the cache manager
     * Also creates the file on the backend if necessary
     * @param index the starting index of the page list
     * @param pages list of pages to flush - may be empty
     * @return the total number of bytes written to the backend
     */
    size_t FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages, const UniqueLock& flushLock);

    /** Asserts the file on the backend has the proper size in case we
     * were truncated larger before mBackendExists - MUST HAVE DATALOCKR/W! */
    void FlushTruncate(const UniqueLock& flushLock);

    Debug mDebug;

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;
    /** Pointer to the cache manager to use */
    CacheManager* mCacheMgr { nullptr };
    /** The size of each page - see description in ConfigOptions */
    const size_t mPageSize;
    /** The current size of the file including dirty extending writes */
    uint64_t mFileSize;

    /** The current read-ahead window (number of pages) (dynamic) - NEVER zero */
    std::atomic<size_t> mFetchSize { 1 };
    /** Mutex that protects mFetchSize and mBandwidthHistory */
    std::mutex mFetchSizeMutex;

    /** List of <index,count> pending reads */
    typedef std::list<std::pair<uint64_t, size_t>> PendingMap;
    /** 
     * Map of page index to exception thrown when reading 
     * This is simpler but less efficient than storing a map of FetchPairs,
     * but performance is not critical for the rare failure case
     */
    typedef std::map<uint64_t, std::exception_ptr> FailureMap;

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of page ranges being downloaded */
    PendingMap mPendingPages;
    /** Map of failures encountered while downloading pages */
    FailureMap mFailedPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;

    /** Primary shared mutex that protects the manager and data */
    SharedMutex mDataMutex;
    /** Shared mutex that is grabbed exclusively when this class is destructed */
    std::shared_mutex mScopeMutex;
    /** Mutex that protects the page maps between concurrent readers (not needed for writers) */
    std::mutex mPagesMutex;
    /** Don't want overlapping flushes, could duplicate writes */
    std::mutex mFlushMutex;

    /** Bandwidth measurement tool for mFetchSize */
    BandwidthMeasure mBandwidth;
    /** Page to/from backend interface */
    PageBackend& mPageBackend;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
