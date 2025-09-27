#ifndef A2UTIL_OPTIONS_H_
#define A2UTIL_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    struct ConfigOptions;
    namespace Account { struct SessionOptions; }
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaUtil {

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
     * @param[out] sessionOptions SessionOptions options ref to fill
     */
    Options(Andromeda::ConfigOptions& configOptions, 
            Andromeda::Backend::HTTPOptions& httpOptions, 
            Andromeda::Backend::RunnerOptions& runnerOptions,
            Andromeda::Account::SessionOptions& sessionOptions);

    bool AddFlag(const std::string& flag) override;

    bool AddOption(const std::string& option, const std::string& value) override;

    void Validate() override;

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
    // TODO RAY !! not just be a struct? would be consistent with the other options classes

private:

    Andromeda::ConfigOptions& mConfigOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::HTTPOptions& mHttpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& mRunnerOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Account::SessionOptions& mSessionOptions; // cppcheck-suppress uninitMemberVarPrivate

    ApiType mApiType { ApiType::API_INVALID };
    std::string mApiPath;
};

} // namespace AndromedaUtil

#endif // A2UTIL_OPTIONS_H_
