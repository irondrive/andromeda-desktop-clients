#include <array>

#include "nlohmann/json.hpp"

#include "Config.hpp"
#include "BackendImpl.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
Config::Config(BackendImpl& backend) : 
    mDebug(__func__,this), mBackend(backend)
{
    MDBG_INFO("()");

    const nlohmann::json config(mBackend.GetConfigJ());

    try
    {
        const nlohmann::json& coreConfig { config.at("core") };

        // parse the major API version
        const std::string apiver { coreConfig.at("apiver").get<std::string>() };
        const StringUtil::StringList apivers { StringUtil::explode(apiver,".") };
        if (apivers.empty()) 
            throw APIVersionException(apiver);

        try
        {
            const unsigned apimaj = static_cast<unsigned>(stoul(apivers[0]));
            if (apimaj != API_MAJOR_VERSION)
                throw APIVersionException(apimaj);
        }
        catch (const std::logic_error& e) { 
            throw APIVersionException(apiver); }


        // check that the required apps are enabled
        const nlohmann::json& appsHave { coreConfig.at("apps") };
        static constexpr std::array<const char*,3> appsReq { "core", "accounts", "files" };

        for (const char* const appReq : appsReq)
            if (!appsHave.contains(appReq))
                throw AppMissingException(appReq);


        // can't get_to() with std::atomic
        mReadOnly.store(coreConfig.at("read_only").get<bool>());

        const nlohmann::json& filesConfig { config.at("files") };
        
        const nlohmann::json& maxbytes { filesConfig.at("upload_maxbytes") };
        if (!maxbytes.is_null()) mUploadMaxBytes.store(maxbytes.get<size_t>());

        // TODO the server also has crchunksize... what to do with that?
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Config::LoadFilesPolicy(BackendImpl& backend)
{
    MDBG_INFO("()");

    /*nlohmann::json policy(backend.GetFilesPolicy());

    try
    {
        if (policy != nullptr)
        {
            mRandWrite.store(policy.at("features").at("randomwrite").get<bool>());
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }*/ // TODO POLICY
}

} // namespace Backend
} // namespace Andromeda
