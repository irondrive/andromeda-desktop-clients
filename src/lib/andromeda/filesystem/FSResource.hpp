
#ifndef LIBA2_FSRESOURCE_H_
#define LIBA2_FSRESOURCE_H_

namespace Andromeda {
namespace Backend { class BackendImpl; }

namespace Filesystem {
namespace Filedata { class CacheManager; class MemoryAllocator; }

/** Collection of resources needed for the filesystem */
struct FSResource
{
    inline FSResource(Backend::BackendImpl& backend_, Filedata::CacheManager* cacheMgr_, Filedata::MemoryAllocator& pageAlloc_):
        backend(backend_), cacheMgr(cacheMgr_), pageAlloc(pageAlloc_){ }

    Backend::BackendImpl& backend;
    Filedata::CacheManager* const cacheMgr;
    Filedata::MemoryAllocator& pageAlloc;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FSRESOURCE_H_