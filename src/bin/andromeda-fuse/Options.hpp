#ifndef A2FUSE_OPTIONS_H_
#define A2FUSE_OPTIONS_H_

#include <string>

#include "andromeda-fuse/FuseAdapter.hpp"
#include "andromeda-fuse/FuseOptions.hpp"

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    struct ConfigOptions;
    namespace Account { struct SessionOptions; }
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaFuse {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param[out] configOptions Config options ref to fill
     * @param[out] httpOptions HTTPRunner options ref to fill
     * @param[out] runnerOptions BaseRunner options ref to fill
     * @param[out] sessionOptions Session options ref to fill
     * @param[out] cacheOptions CacheManager options ref to fill
     * @param[out] fuseOptions FUSE options ref to fill
     */
    Options(Andromeda::ConfigOptions& configOptions, 
            Andromeda::Backend::HTTPOptions& httpOptions, 
            Andromeda::Backend::RunnerOptions& runnerOptions,
            Andromeda::Account::SessionOptions& sessionOptions,
            Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions,
            AndromedaFuse::FuseOptions& fuseOptions);

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;
    void TryAddUrlOption(const std::string& option, const std::string& value) override;
    void Validate() const override;

    /** Backend connection type */
    enum class ApiType : uint8_t
    {
        API_URL,
        API_PATH,

        API_INVALID
    };

    /** Returns the specified API type */
    [[nodiscard]] ApiType GetApiType() const { return mApiType; }

    /** Returns the path to the API endpoint */
    [[nodiscard]] const std::string& GetApiPath() const { return mApiPath; }

    /** Folder types that can be mounted as root */
    enum class RootType : uint8_t
    {
        SUPERROOT,
        STORAGE,
        FOLDER
    };

    /** Returns the filesystem directory to mount */
    [[nodiscard]] const std::string& GetMountPath() const { return mMountPath; }

    /** Returns the specified mount item type */
    [[nodiscard]] RootType GetMountRootType() const { return mMountRootType; }

    /** Returns the specified mount item ID */
    [[nodiscard]] const std::string& GetMountItemID() const { return mMountItemID; }

    /** Returns true if we should run in the foreground */
    [[nodiscard]] bool isForeground() const { return mForeground; }

private:

    Andromeda::ConfigOptions& mConfigOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::HTTPOptions& mHttpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& mRunnerOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Account::SessionOptions& mSessionOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Filesystem::Filedata::CacheOptions& mCacheOptions; // cppcheck-suppress uninitMemberVarPrivate
    AndromedaFuse::FuseOptions& mFuseOptions; // cppcheck-suppress uninitMemberVarPrivate

    ApiType mApiType { ApiType::API_INVALID };
    std::string mApiPath;

    std::string mMountPath;

    RootType mMountRootType { RootType::SUPERROOT };
    std::string mMountItemID;

    bool mForeground { false };
};

} // namespace AndromedaFuse

#endif // A2FUSE_OPTIONS_H_
