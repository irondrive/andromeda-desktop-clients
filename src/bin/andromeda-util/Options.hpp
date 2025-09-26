#ifndef A2UTIL_OPTIONS_H_
#define A2UTIL_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    struct ConfigOptions;
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
     */
    Options(Andromeda::ConfigOptions& configOptions, 
            Andromeda::Backend::HTTPOptions& httpOptions, 
            Andromeda::Backend::RunnerOptions& runnerOptions);

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

    /** Returns true if a username is specified */
    [[nodiscard]] bool HasUsername() const { return !mUsername.empty(); }

    /** Returns the specified username */
    [[nodiscard]] const std::string& GetUsername() const { return mUsername; }

    /** Returns true if a password is specified */
    [[nodiscard]] bool HasPassword() const { return !mPassword.empty(); }

    /** Returns the specified password */
    [[nodiscard]] const std::string& GetPassword() const { return mPassword; }

    /** Returns true if a session ID was provided */
    [[nodiscard]] bool HasSession() const { return !mSessionid.empty(); }

    /** Returns the specified session ID */
    [[nodiscard]] const std::string& GetSessionID() const { return mSessionid; }

    /** Returns the specified session key */
    [[nodiscard]] const std::string& GetSessionKey() const { return mSessionkey; } // TODO RAY !! can this be commonized with fuse? perhaps move to SessionOptions or just Session?

    /** Returns true if using a session is forced */
    [[nodiscard]] bool GetForceSession() const { return mForceSession; } // TODO RAY !! don't think this applies?

private:

    Andromeda::ConfigOptions& mConfigOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::HTTPOptions& mHttpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& mRunnerOptions; // cppcheck-suppress uninitMemberVarPrivate

    ApiType mApiType { ApiType::API_INVALID };
    std::string mApiPath;

    std::string mUsername;
    std::string mPassword;
    bool mForceSession { false };

    std::string mSessionid;
    std::string mSessionkey;
};

} // namespace AndromedaUtil

#endif // A2UTIL_OPTIONS_H_
