#ifndef A2UTIL_OPTIONS_H_
#define A2UTIL_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Account { struct SessionOptions; }
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaUtil {

/** Manages command line options and config */
struct Options : public Andromeda::BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param[out] httpOptions HTTPRunner options ref to fill
     * @param[out] runnerOptions BaseRunner options ref to fill
     * @param[out] sessionOptions SessionOptions options ref to fill
     */
    Options(Andromeda::Backend::HTTPOptions& httpOptions_, 
            Andromeda::Backend::RunnerOptions& runnerOptions_,
            Andromeda::Account::SessionOptions& sessionOptions_);

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;

    /** Backend connection type */
    enum class ApiType : uint8_t
    {
        API_URL,
        API_PATH,

        API_INVALID
    };

    Andromeda::Backend::HTTPOptions& httpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& runnerOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Account::SessionOptions& sessionOptions; // cppcheck-suppress uninitMemberVarPrivate

    /** Returns the specified API type */
    ApiType apiType { ApiType::API_INVALID };
    /** Returns the path to the API endpoint */
    std::string apiPath;
};

} // namespace AndromedaUtil

#endif // A2UTIL_OPTIONS_H_
