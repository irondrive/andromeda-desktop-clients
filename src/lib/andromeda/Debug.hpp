#ifndef LIBA2_DEBUG_H_
#define LIBA2_DEBUG_H_

#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>

namespace Andromeda {

/** Global thread-safe debug printing */
class Debug
{
public:

    /** Debug verbosity */
    enum class Level
    {
        /** Only show Error()s */ ERRORS,  
        /** Also show Backend */  BACKEND, 
        /** Everything else */    INFO,    
        /** Show extra details */ DETAILS,
        LAST = DETAILS
    };

    /** Returns the configured global debug level */
    static Level GetLevel(){ return sLevel; }

    /** Sets the configured global debug level */
    static void SetLevel(Level level){ sLevel = level; }

    /** Adds the given component name to the filter set */
    static void AddFilter(const std::string& name){ sPrefixes.emplace(name); }

    /**
     * @param prefix to use for all prints
     * @param addr address to print with details
     */
    explicit Debug(const std::string& prefix, void* addr) noexcept : 
        mAddr(addr), mPrefix(prefix) { }

    /** Function to send debug text to a given output stream */
    using StreamFunc = std::function<void (std::ostream&)>;

    /** Prints func to cerr if the level is >= ERRORS */
    inline void Error(const StreamFunc& strfunc)
    {
        if (sLevel >= Level::ERRORS) Print(strfunc);
    }

    /** Prints func to cerr if the level is >= BACKEND */
    inline void Backend(const StreamFunc& strfunc)
    {
        if (sLevel >= Level::BACKEND) PrintIf(strfunc);
    }

    /** Prints func to cerr if the level is >= INFO */
    inline void Info(const StreamFunc& strfunc)
    {
        if (sLevel >= Level::INFO) PrintIf(strfunc);
    }

    /** Syntactic sugar to send the current function name and strcode to debug (error) */
    #define DBG_ERROR(debug, strcode) { static const std::string myfname(__func__); \
        debug.Error([&](std::ostream& str){ str << myfname << strcode; }); }

    /** Syntactic sugar to send the current function name and strcode to debug (info) */
    #define DBG_INFO(debug, strcode) { static const std::string myfname(__func__); \
        debug.Info([&](std::ostream& str){ str << myfname << strcode; }); }

    #define DDBG_ERROR(strfunc) DBG_ERROR(debug, strfunc)
    #define MDBG_ERROR(strfunc) DBG_ERROR(mDebug, strfunc)
    #define SDBG_ERROR(strfunc) DBG_ERROR(sDebug, strfunc)

    #define DDBG_INFO(strfunc) DBG_INFO(debug, strfunc)
    #define MDBG_INFO(strfunc) DBG_INFO(mDebug, strfunc)
    #define SDBG_INFO(strfunc) DBG_INFO(sDebug, strfunc)

    /**
     * Returns a StreamFunc to print a struct as hex to the buffer
     * @param ptr address of struct to print
     * @param bytes number of bytes to print
     * @param width number of bytes per line
     */
    static StreamFunc DumpBytes(const void* ptr, size_t bytes, size_t width = 16);

private:

    /** Prints func to cerr with other info IF the prefix is enabled - THREAD SAFE */
    void PrintIf(const StreamFunc& strfunc);

    /** Prints func to cerr with other info - THREAD SAFE */
    void Print(const StreamFunc& strfunc);

    /** The address this debug instance belongs to */
    void* mAddr { nullptr };
    
    /** The module name this debug instance belongs to */
    std::string mPrefix;

    /** Global debug level */
    static Level sLevel;

    /** Set of prefixes to filter printing */
    static std::unordered_set<std::string> sPrefixes;
};

} // namespace Andromeda

#endif // LIBA2_DEBUG_H_
