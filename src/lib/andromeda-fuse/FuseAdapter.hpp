#ifndef A2FUSE_FUSEADAPTER_H_
#define A2FUSE_FUSEADAPTER_H_

#include <condition_variable>
#include <exception>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "FuseOptions.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Filesystem { 
    class Folder;
} // nanespace Filesystem
} // namespace Andromeda

namespace AndromedaFuse {

struct FuseContext;
struct FuseMount;
struct FuseOperations;

/** Static class for FUSE operations */
class FuseAdapter
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Andromeda::BaseException 
    { public:
        /** @param message FUSE error message */
        explicit Exception(const std::string& message) :
            Andromeda::BaseException("FUSE Error: "+message) {};
        /** 
         * @param message FUSE error message 
         * @param retval -errno error code
         */
        explicit Exception(const std::string& message, int retval) :
            Andromeda::BaseException("FUSE Error: "+message+": "
                +Andromeda::Utilities::GetErrorString(-retval)) {};
    };

    /** Thread mode for the FUSE Adapter */
    enum class RunMode
    {
        /** Run in the foreground (block) */
        FOREGROUND,
        /** 
         * Run in the foreground but detach from the terminal 
         * NOTE - calls fork() which does NOT clone other existing threads
         */
        DAEMON,
        /** Run in a background thread (don't block) */
        THREAD
    };

    /**
     * Starts and mounts libfuse
     * @param path filesystem path to mount
     * @param root andromeda folder as root
     * @param options command line options (copied)
     */
    FuseAdapter(
        const std::string& path, 
        Andromeda::Filesystem::Folder& root, 
        const FuseOptions& options);

    /** Stop and unmount FUSE */
    virtual ~FuseAdapter();

    /** Function to run after forking (e.g. start threads) */
    typedef std::function<void()> ForkFunc;

    /** 
     * Mounts and starts the FUSE loop, blocks if there is an existing thread
     * @param runMode RunMode enum specifying threading mode
     * @param forkFunc function to run after daemonizing, if runMode is DAEMON
    */
    void StartFuse(RunMode runMode, const ForkFunc& forkFunc = {});

    /** Returns the mounted filesystem path */
    const std::string& GetMountPath() const { return mMountPath; }

    /** Returns the root folder */
    Andromeda::Filesystem::Folder& GetRootFolder() { return mRootFolder; }

    /** Returns the FUSE options */
    const FuseOptions& GetOptions() const { return mOptions; }

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    friend struct FuseOperations;

    /** 
     * Runs/mounts libfuse (blocking) 
     * @param regSignals if true, register FUSE signal handlers
     * @param daemonize if true, fork to a detached process
     * @param forkFunc function to run after daemonizing if applicable
     */
    void FuseLoop(bool regSignals, bool daemonize, const ForkFunc& forkFunc);

    /** Signals initialization complete */
    void SignalInit();
    
    std::string mMountPath;
    Andromeda::Filesystem::Folder& mRootFolder;
    FuseOptions mOptions;

    std::thread mFuseThread;

#if LIBFUSE2
    friend struct FuseContext;
    FuseContext* mFuseContext { nullptr };
#else // !LIBFUSE2
    friend struct FuseMount;
    FuseMount* mFuseMount { nullptr };
#endif // LIBFUSE2

    bool mInitialized { false };
    std::mutex mInitMutex;
    std::condition_variable mInitCV;
    std::exception_ptr mInitError;

    FuseAdapter(const FuseAdapter&) = delete; // no copying
};

} // namespace AndromedaFuse

#endif // A2FUSE_FUSEADAPTER_H
