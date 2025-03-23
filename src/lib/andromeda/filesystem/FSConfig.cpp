#include <mutex>
#include <unordered_map>
#include <utility>
#include "nlohmann/json.hpp"

#include "FSConfig.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {

namespace { // anonymous
using CacheMap = std::unordered_map<std::string, FSConfig>; 
std::mutex sCacheMutex; CacheMap sCache;
} // namespace

/*****************************************************/
const FSConfig& FSConfig::LoadByID(BackendImpl& backend, const std::string& id)
{
    const std::lock_guard<decltype(sCacheMutex)> llock(sCacheMutex);

    CacheMap::iterator it { sCache.find(id) };

    if (it == sCache.end())
    {
        it = sCache.emplace(std::piecewise_construct, std::forward_as_tuple(id), 
            std::forward_as_tuple(backend.GetStorage(id), nlohmann::json{}/*backend.GetStoragePolicy(id) TODO POLICY*/)).first;
    }

    return it->second;
}

/*****************************************************/
FSConfig::FSConfig(const nlohmann::json& data, const nlohmann::json& policy) :
    mDebug(__func__, this)
{
    if (data.is_null() /*&& policy.is_null()*/) return; // TODO POLICY

    try
    {
        if (data.at("chunksize") != nullptr)
            data.at("chunksize").get_to(mChunksize);

        data.at("readonly").get_to(mReadOnly);

        const std::string sttype { data.at("sttype").get<std::string>() };

        if (sttype == "S3")  mWriteMode = WriteMode::UPLOAD;
        if (sttype == "FTP") mWriteMode = WriteMode::APPEND;

        if (mWriteMode >= WriteMode::RANDOM)
        {
            if (policy.contains("features"))
            {
                const nlohmann::json& rw(policy.at("features").at("randomwrite"));

                if (!rw.is_null() && !rw.get<bool>())
                    mWriteMode = WriteMode::APPEND;
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

} // namespace Filesystem
} // namespace Andromeda
