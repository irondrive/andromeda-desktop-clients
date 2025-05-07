
#include <cassert>
#include <memory>
#include <ostream>

#if WIN32
#include <windows.h>
#else // !WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32

#include "MemoryAllocator.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
MemoryAllocator::MemoryAllocator() : 
    mPageSize(calcPageSize()),
    mDebug(__func__,this)
{
    MDBG_INFO("... mPageSize:" << mPageSize);
}

#if DEBUG // sanity checks
/*****************************************************/
MemoryAllocator::~MemoryAllocator(){ assert(mAllocMap.empty()); }
#endif // DEBUG

/*****************************************************/
size_t MemoryAllocator::calcPageSize() const
{
#if WIN32
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    return systemInfo.dwAllocationGranularity;
#else // !WIN32
    return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif // WIN32
}

/*****************************************************/
void* MemoryAllocator::alloc(size_t pages)
{
    if (!pages) return nullptr;

#ifdef FILEDATA_USE_MALLOC
    void* ptr = ::malloc(pages*mPageSize); // NOLINT(*-owning-memory, *-no-malloc)
#elif WIN32
    LPVOID ptr = VirtualAlloc(nullptr, pages*mPageSize, 
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else // !WIN32
    void* ptr = mmap(nullptr, pages*mPageSize, 
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif // WIN32
    MDBG_INFO("(ptr:" << ptr << " pages:" << pages << " bytes:" << pages*mPageSize << ")");

#if DEBUG // sanity checks
{ // lock scope
    const LockGuard lock(mMutex);
    mAllocMap.emplace(ptr, pages);
}
#endif // DEBUG

    stats(__func__, pages, true);
    return ptr;
}

/*****************************************************/
void MemoryAllocator::free(void* const ptr, const size_t pages)
{
    MDBG_INFO("(ptr:" << ptr << " pages:" << pages << " bytes:" << pages*mPageSize << ")");
    if (ptr == nullptr) return;

#if DEBUG // sanity checks
{ // lock scope
    const LockGuard lock(mMutex);
    // lower_bound with map<greater> means first ptr <= our ptr
    AllocMap::iterator it { mAllocMap.lower_bound(ptr) };
    assert(it != mAllocMap.end());

    MDBG_INFO("... entry ptr:" << it->first << " pages:" << it->second);
    uint8_t* const ptrEntry { static_cast<uint8_t*>(it->first) };
    const size_t pagesEntry { it->second };

    uint8_t* const ptrFree { static_cast<uint8_t*>(ptr) };
    const size_t pagesFree { pages };
    assert(ptrEntry <= ptrFree);

    // the given pointer must be page-boundary aligned
    assert(static_cast<size_t>(ptrFree-ptrEntry) % mPageSize == 0);
    // the requested free range should be covered
    assert(ptrEntry + pagesEntry*mPageSize >= ptrFree + pagesFree*mPageSize);

    mAllocMap.erase(it); // delete old entry

    if (ptrEntry < ptrFree)
    {
        const size_t pagesBefore { static_cast<size_t>(ptrFree-ptrEntry)/mPageSize };
        MDBG_INFO("... ptr:" << static_cast<void*>(ptrEntry) << " pagesBefore:" << pagesBefore);
        mAllocMap.emplace(ptrEntry, pagesBefore);
    }

    uint8_t* const afterStart { ptrFree+pagesFree*mPageSize };
    const uint8_t* const afterEnd { ptrEntry+pagesEntry*mPageSize };
    if (afterEnd > afterStart)
    {
        const size_t pagesAfter { static_cast<size_t>(afterEnd-afterStart)/mPageSize };
        MDBG_INFO("... ptr:" << static_cast<void*>(afterStart) << " pagesAfter:" << pagesAfter);
        mAllocMap.emplace(afterStart, pagesAfter);
    }
}
#endif // DEBUG

#ifdef FILEDATA_USE_MALLOC
    ::free(ptr); // NOLINT(*-owning-memory, *-no-malloc)
#elif WIN32
    VirtualFree(ptr, pages*mPageSize, MEM_RELEASE);
#else // !WIN32
    munmap(ptr, pages*mPageSize);
#endif // WIN32

    stats(__func__, pages, false);
}

/*****************************************************/
void MemoryAllocator::stats(const char* const fname, const size_t pages, bool alloc) 
{
    if (mDebug.GetLevel() >= Debug::Level::INFO)
    {
        const LockGuard lock(mMutex); // our lock must be before printing
        mDebug.Info([&](std::ostream& str)
        {
            if (alloc) { ++mAllocs; mTotalPages += pages; mTotalBytes += pages*mPageSize; }
            else       { ++mFrees; mTotalPages -= pages; mTotalBytes -= pages*mPageSize; }

            str << fname << "... mTotalPages:" << mTotalPages << " mTotalBytes:" << mTotalBytes
        #if DEBUG // sanity checks
            << " mAllocMap:" << mAllocMap.size()
        #endif // DEBUG
            << " mAllocs:" << mAllocs << " mFrees:" << mFrees; 
        });
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
