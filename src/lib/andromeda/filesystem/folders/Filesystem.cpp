#include "nlohmann/json.hpp"

#include "Filesystem.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/FSConfig.hpp"
#include "andromeda/filesystem/FSResource.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadByID(FSResource& fsResource, const std::string& fsid)
{
    const nlohmann::json data(fsResource.backend.GetStorage(fsid));

    return std::make_unique<Filesystem>(fsResource, data, nullptr);
}

/*****************************************************/
Filesystem::Filesystem(FSResource& fsResource, const nlohmann::json& data, Folder* parent) :
    PlainFolder(fsResource, data, parent), mFsid(mId), mDebug(__func__,this) 
{
    MDBG_INFO("()");

    mId = "";
    // our JSON ID is the storage ID, use mId for the root folder
    
    // NOTE we use the storage object as our "item" and lazy load the real folder object 
    // only when needed, both to avoid an unnecessary request that might not be needed
    // and also so that this object can still be loaded when the storage has a configuration issue

    mStConfig = &FSConfig::LoadByID(mBackend, mFsid);
}

/*****************************************************/
const std::string& Filesystem::GetID()
{
    const UniqueLock idLock(mIdMutex); // lazy-load the folder ID
    if (mId.empty()) LoadID(mBackend.GetRootFolder(mFsid), idLock);

    return mId;
}

/*****************************************************/
void Filesystem::LoadID(const nlohmann::json& data, const UniqueLock& idLock)
{
    try
    {
        data.at("id").get_to(mId);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Filesystem::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    const nlohmann::json data(mBackend.GetRootFolder(mFsid));

    { // lock scope
        const UniqueLock idLock(mIdMutex);
        if (mId.empty()) LoadID(data, idLock);
    }

    LoadItemsFrom(data, itemsLocks, thisLock);
}

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda
