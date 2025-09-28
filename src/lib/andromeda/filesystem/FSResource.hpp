
#ifndef LIBA2_FSRESOURCE_H_
#define LIBA2_FSRESOURCE_H_

#include "FSOptions.hpp"
#include "andromeda/backend/BackendImpl.hpp"

namespace Andromeda {
namespace Backend { class BackendImpl; }

namespace Filesystem {
namespace Filedata { class CacheManager; class MemoryAllocator; }

/** Collection of resources needed for the filesystem */
struct FSResource
{
    /** Side effect - applies memory/RO from fsOptions to backend */
    inline FSResource(FSOptions& options_, Backend::BackendImpl& backend_, Filedata::CacheManager* cacheMgr_, Filedata::MemoryAllocator& pageAlloc_):
        options(options_), backend(backend_), cacheMgr(cacheMgr_), pageAlloc(pageAlloc_)
    {
        backend.setIsMemory(options.cacheType == FSOptions::CacheType::MEMORY);
        backend.setIsReadOnly(options.readOnly);
    }

    FSOptions options; // stored by value!
    Backend::BackendImpl& backend;
    Filedata::CacheManager* const cacheMgr;
    Filedata::MemoryAllocator& pageAlloc;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FSRESOURCE_H_