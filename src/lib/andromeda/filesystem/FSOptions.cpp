
#include <sstream>

#include "FSOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda::Filesystem {

/*****************************************************/
std::string FSOptions::HelpText()
{
    std::ostringstream output;
    const FSOptions optDefault;

    const auto defRefresh(optDefault.refreshTime.count());
    const auto defReadAhead(optDefault.readAheadTime.count());
    const size_t stBits { sizeof(size_t)*8 };

    using std::endl; output 
        << "FS Advanced:     [-r|--read-only] [--dir-refresh secs(" << defRefresh << ")] [--cachemode none|memory|normal]" << endl
        << "Data Advanced:   [--pagesize bytes"<<stBits<<"(" << StringUtil::bytesToString(optDefault.pageSize) << ")] [--read-ahead ms(" << defReadAhead << ")]"
            << " [--read-max-cache-frac uint32(" << optDefault.readMaxCacheFrac << ")] [--read-ahead-buffer pages(" << optDefault.readAheadBuffer << ")]";

    return output.str();
}

/*****************************************************/
bool FSOptions::AddFlag(const std::string& flag)
{
    if (flag == "r" || flag == "read-only")
        readOnly = true;
    else return false; // not used

    return true;
}

/*****************************************************/
bool FSOptions::AddOption(const std::string& option, const std::string& value) // NOLINT(readability-function-cognitive-complexity)
{
    if (option == "cachemode")
    {
        if      (value == "none")   cacheType = FSOptions::CacheType::NONE;
        else if (value == "memory") cacheType = FSOptions::CacheType::MEMORY;
        else if (value == "normal") cacheType = FSOptions::CacheType::NORMAL;
        else throw BaseOptions::BadValueException(option);
    }
    else if (option == "dir-refresh")
    {
        refreshTime = static_cast<decltype(refreshTime)>(GetUnsigned(option,value));
    }
    else if (option == "pagesize")
    {
        try { pageSize = static_cast<decltype(pageSize)>(StringUtil::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!pageSize) throw BaseOptions::BadValueException(option);
    }
    else if (option == "read-ahead")
    {
        readAheadTime = static_cast<decltype(readAheadTime)>(GetUnsigned(option,value));
    }
    else if (option == "read-max-cache-frac")
    {
        readMaxCacheFrac = static_cast<decltype(readMaxCacheFrac)>(GetUnsigned(option,value,false));
    }
    else if (option == "read-ahead-buffer")
    {
        readAheadBuffer = static_cast<decltype(readAheadBuffer)>(GetUnsigned(option,value));
    }
    else return false; // not used

    return true; 
}

} // namespace Andromeda::Filesystem
