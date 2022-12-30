#include <nlohmann/json.hpp>

#include "Adopted.hpp"
#include "Filesystems.hpp"
#include "SuperRoot.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
SuperRoot::SuperRoot(BackendImpl& backend) : 
    Folder(backend), mDebug("SuperRoot",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    backend.RequireAuthentication();

    mName = "SuperRoot";
}

/*****************************************************/
void SuperRoot::LoadItems()
{
    if (HaveItems()) return; // never refresh

    mDebug << __func__ << "()"; mDebug.Info();

    std::unique_ptr<Adopted> adopted(std::make_unique<Adopted>(mBackend, *this));
    mItemMap[adopted->GetName()] = std::move(adopted);

    std::unique_ptr<Filesystems> filesystems(std::make_unique<Filesystems>(mBackend, *this));
    mItemMap[filesystems->GetName()] = std::move(filesystems);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders