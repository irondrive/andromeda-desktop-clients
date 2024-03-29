

#include <cstring>

#include "CachingAllocator.hpp"
#include "Page.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
Page::Page(size_t pageSize, CachingAllocator& memAlloc) : 
    mAlloc(memAlloc), 
    mBytes(pageSize), 
    mPages(mAlloc.getNumPages(mBytes)), 
    mData(mPages ? static_cast<char*>(mAlloc.alloc(mPages)) : nullptr){ }

/*****************************************************/
Page::Page(Page&& page) noexcept : // move constructor
    mAlloc(page.mAlloc), 
    mBytes(page.mBytes), 
    mPages(page.mPages), 
    mData(page.mData)
{
    page.mBytes = 0;
    page.mPages = 0;
    page.mData = nullptr;
}

/*****************************************************/
Page::~Page()
{
    if (mData != nullptr)
        mAlloc.free(mData, mPages);
}

/*****************************************************/
size_t Page::capacity() const
{ 
    return mPages*mAlloc.getPageSize(); 
}

/*****************************************************/
void Page::resize(size_t newBytes)
{
    const size_t newPages { mAlloc.getNumPages(newBytes) };
    if (newPages != mPages) // re-allocate
    {
        char* const newData { static_cast<char*>(mAlloc.alloc(newPages)) };
        if (mData != nullptr)
        {
            std::memcpy(newData, mData, std::min(newBytes,mBytes));
            mAlloc.free(mData, mPages);
        }

        mBytes = newBytes;
        mPages = newPages;
        mData = newData;
    }
    else mBytes = newBytes;
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
