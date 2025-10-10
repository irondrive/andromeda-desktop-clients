#ifndef A2FUSE_OPTIONS_H_
#define A2FUSE_OPTIONS_H_

#include <string>

#include "andromeda-fuse/FuseAdapter.hpp"
#include "andromeda-fuse/FuseOptions.hpp"

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Account { struct SessionOptions; }
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
    namespace Filesystem { struct FSOptions; namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaFuse {

/** Manages command line options and config */
struct Options : public Andromeda::BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param[out] httpOptions HTTPRunner options ref to fill
     * @param[out] runnerOptions BaseRunner options ref to fill
     * @param[out] sessionOptions Session options ref to fill
     * @param[out] fsOptions Filesystem options ref to fill
     * @param[out] cacheOptions CacheManager options ref to fill
     * @param[out] fuseOptions FUSE options ref to fill
     */
    Options(Andromeda::Backend::HTTPOptions& httpOptions_, 
            Andromeda::Backend::RunnerOptions& runnerOptions_,
            Andromeda::Account::SessionOptions& sessionOptions_,
            Andromeda::Filesystem::FSOptions& fsOptions_, 
            Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions_,
            AndromedaFuse::FuseOptions& fuseOptions_);

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

    /** Folder types that can be mounted as root */
    enum class RootType : uint8_t
    {
        SUPERROOT,
        STORAGE,
        FOLDER
    };

    Andromeda::Backend::HTTPOptions& httpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& runnerOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Account::SessionOptions& sessionOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Filesystem::FSOptions& fsOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions; // cppcheck-suppress uninitMemberVarPrivate
    AndromedaFuse::FuseOptions& fuseOptions; // cppcheck-suppress uninitMemberVarPrivate

    /** Returns the specified API type */
    ApiType apiType { ApiType::API_INVALID };
    /** Returns the path to the API endpoint if CLI */
    std::string apiPath;
    /** Returns the URL to the API endpoint if HTTP */
    std::string apiUrl;

    /** Returns the filesystem directory to mount */
    std::string mountPath;

    /** Returns the specified mount item type */
    RootType mountRootType { RootType::SUPERROOT };
    /** Returns the specified mount item ID */
    std::string mountItemID;

    /** Returns true if we should run in the foreground */
    bool foreground { false };
};

} // namespace AndromedaFuse

#endif // A2FUSE_OPTIONS_H_
