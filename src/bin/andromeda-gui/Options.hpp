#ifndef A2GUI_OPTIONS_H_
#define A2GUI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaGui {

/** Manages command line options and config */
struct Options : public Andromeda::BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** @param[out] cacheOptions CacheManager options ref to fill */
    explicit Options(Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions_);

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;

    Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions; // cppcheck-suppress uninitMemberVarPrivate
};

} // namespace AndromedaGui

#endif // A2GUI_OPTIONS_H_
