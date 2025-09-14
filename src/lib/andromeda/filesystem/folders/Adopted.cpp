#include "nlohmann/json.hpp"

#include "Adopted.hpp"
#include "andromeda/backend/BackendImpl.hpp"
#include "andromeda/filesystem/Folder.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
Adopted::Adopted(FSResource& fsResource, Folder& parent) :
    PlainFolder(fsResource, &parent), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mName = "Adopted by others";
}

/*****************************************************/
void Adopted::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    LoadItemsFrom(mBackend.GetAdopted(), itemsLocks, thisLock);
}

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda
