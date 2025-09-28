#ifndef LIBA2_RUNNEROPTIONS_H_
#define LIBA2_RUNNEROPTIONS_H_

#include <chrono>
#include <cstdint>
#include <string>
#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
namespace Backend {

/** Runner config options */
struct RunnerOptions : public BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    bool AddFlag(const std::string& flag) override { return false; }
    bool AddOption(const std::string& option, const std::string& value) override;

    using seconds = std::chrono::seconds;

    /** maximum retries before throwing */
    uint32_t maxRetries { 4 };
    /** The time to wait between each retry */
    seconds retryTime { 3 };
    /** The connection read/write timeout */
    seconds timeout { 60 };
    /** Buffer/chunk size when reading file streams */
    size_t streamBufferSize { 1048576 }; // 1M

    /** The maximum number of concurrent backend runners, never zero! */
    size_t poolSize { 1 }; // TODO server has threading issues
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNEROPTIONS_H_
