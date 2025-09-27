
#include <sstream>

#include "CacheOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
std::string CacheOptions::HelpText()
{
    std::ostringstream output;
    const CacheOptions optDefault;

    const auto defDirty(milliseconds(optDefault.maxDirtyTime).count());
    const size_t stBits { sizeof(size_t)*8 };

    output << "Cache Advanced:  [--no-cachemgr] [--max-dirty ms(" << defDirty << ")]"
        << " [--memory-limit bytes"<<stBits<<"(" << StringUtil::bytesToString(optDefault.memoryLimit) << ")]"
        << " [--evict-frac uint32(" << optDefault.evictSizeFrac << ")]";

    return output.str();
}

/*****************************************************/
bool CacheOptions::AddFlag(const std::string& flag)
{
    if (flag == "no-cachemgr")
        disable = true;
    else return false; // not used

    return true;
}

/*****************************************************/
bool CacheOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "max-dirty")
    {
        maxDirtyTime = static_cast<decltype(maxDirtyTime)>(GetUnsigned(option,value));
    }
    else if (option == "memory-limit")
    {
        try { memoryLimit = static_cast<size_t>(StringUtil::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "evict-frac")
    {
        evictSizeFrac = static_cast<decltype(evictSizeFrac)>(GetUnsigned(option,value,false));
    }
    else return false; // not used

    return true; 
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
