#ifndef LIBA2_CACHEOPTIONS_H_
#define LIBA2_CACHEOPTIONS_H_

#include <chrono>
#include <cstdint>
#include <string>

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** CacheManager options */
struct CacheOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag);

    /** 
     * Adds the given option/value, returning true iff it was used
     * @throws BaseOptions::Exception if invalid arguments
     */
    bool AddOption(const std::string& option, const std::string& value);

    /** 
     * The maxmimum total file data cached in memory before evicting (bytes) 
     * Larger values consume more memory while increasing cache effectiveness.
     * While it may be tempting to set this to 0, keep in mind reads are orders of magnitude
     * faster when multi-page readAheads can happen, and a readAhead can be larger than some fraction of this 
     * (see ConfigOptions.readMaxCacheFrac) even small values e.g. 8MB make a huge difference in performance.
     */
    size_t memoryLimit { static_cast<size_t>(256)*1024*1024 };

    /** 
     * The fraction of mMemoryLimit to get below the max when evicting .
     * E.g. if limit=256M and frac=32, evict starts at 256M and evicts 8M of pages
     * Smaller values may result in less CPU at the expense of less effective cache
     */
    uint32_t evictSizeFrac { 16 };

    using milliseconds = std::chrono::milliseconds;

    /** 
     * The max amount of dirty data to have in memory in terms of transfer time.
     * Bandwidth measuring is used to convert this time-target to an actual byte count.
     * Larger values may improve performance but increase memory usage and risk losing more data if we crash or the server goes down, etc.
     */
    milliseconds maxDirtyTime { 1000 };

    /** True to disable the CacheManager */
    bool disable { false };
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEOPTIONS_H_
