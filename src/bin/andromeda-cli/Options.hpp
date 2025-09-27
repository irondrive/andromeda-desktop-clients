#ifndef A2CLI_OPTIONS_H_
#define A2CLI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
}

namespace AndromedaCli {

/** Manages command line options and config */
struct Options : public Andromeda::BaseOptions
{
    /** Retrieve the base usage help text string */
    static std::string CoreHelpText();

    /** Retrieve the main command help text string */
    static std::string MainHelpText();

    /** Retrieve the detailed options help text string */
    static std::string DetailHelpText();

    /** 
     * @param[out] httpOptions HTTPRunner options ref to fill 
     * @param[out] runnerOptions BaseRunner options ref to fill
     */
    Options(
        Andromeda::Backend::HTTPOptions& httpOptions_,
        Andromeda::Backend::RunnerOptions& runnerOptions_);

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;
    void Validate() const override;

    Andromeda::Backend::HTTPOptions& httpOptions; // cppcheck-suppress uninitMemberVarPrivate
    Andromeda::Backend::RunnerOptions& runnerOptions; // cppcheck-suppress uninitMemberVarPrivate

    /** Returns the URL of the API endpoint */
    std::string apiUrl;
    /** Returns true if output streaming is requested */
    bool streamOut { false };
    /** Returns true if unsafe URLs are allowed */
    bool allowUnsafeUrl { false };
};

} // namespace AndromedaCli

#endif // A2CLI_OPTIONS_H_
