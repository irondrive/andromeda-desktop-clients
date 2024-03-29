
#ifndef LIBA2_BANDWIDTHMEASURE_H_
#define LIBA2_BANDWIDTHMEASURE_H_

#include <array>
#include <chrono>

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** 
 * Keeps a history of bandwidth measurements to
 * calculate the ideal size for network transfers
 * NOT THREAD SAFE (protect externally)
 */
class BandwidthMeasure
{
public:

    using milliseconds = std::chrono::milliseconds;

    /** @param timeTarget reference to the target time */
    explicit BandwidthMeasure(const char* debugName, const milliseconds& timeTarget);

    /** Updates the bandwidth history with the given measure and returns the estimated targetBytes */
    size_t UpdateBandwidth(size_t bytes, const std::chrono::steady_clock::duration& time);

private:

    /** The desired time target for the transfer */
    const milliseconds& mTimeTarget;
    
    /** The number of bandwidth history entries to store */
    static constexpr size_t BANDWIDTH_WINDOW { 4 };

    /** Array of calculated bandwidth targetBytes to average together */
    std::array<size_t, BANDWIDTH_WINDOW> mBandwidthHistory { };

    /** The next index of mBandwidthHistory to write with a measurement */
    size_t mBandwidthHistoryIdx { 0 };

    mutable Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_BANDWIDTHMEASURE_H_
