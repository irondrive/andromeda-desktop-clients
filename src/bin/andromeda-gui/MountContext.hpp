#ifndef A2GUI_MOUNTCONTEXT_H
#define A2GUI_MOUNTCONTEXT_H

#include <filesystem>
#include <memory>
#include <string>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"

namespace Andromeda { 
    namespace Backend { class BackendImpl; }
    namespace Filesystem { class Folder; }
}

namespace AndromedaGui {

/** Encapsulates a FUSE mount and root folder */
class MountContext
{
public:

    /** Base Exception for mount context issues */
    class Exception : public Andromeda::BaseException { public:
        /** @param message error message */
        explicit Exception(const std::string& message) :
            Andromeda::BaseException("Mount Error: "+message) {}; };

    /** Exception indicating that no home directory was found */
    class UnknownHomeException : public Exception { public:
        UnknownHomeException() : Exception("Unknown Home Directory") {}; };

    /** Exception indicating the desired mount directory is not empty */
    class NonEmptyMountException : public Exception { public:
        explicit NonEmptyMountException(const std::string& path) : 
            Exception("Mount Directory not empty:\n\n"+path) {}; };

    /** Exception indicating std::filesystem threw an exception */
    class FilesystemErrorException : public Exception { public:
        explicit FilesystemErrorException(const std::filesystem::filesystem_error& err) : 
            Exception("Filesystem Error: "+std::string(err.what())) {}; };

    /**
     * Create a new MountContext
     * @param backend the backend resource to use
     * @param homeRel if true, mountPath is $HOME-relative
     * @param mountPath filesystem path to mount - must already exist if not homeRel
     * @param options FUSE adapter options
     */
    MountContext(Andromeda::Backend::BackendImpl& backend,
        bool homeRel, std::string mountPath, 
        AndromedaFuse::FuseOptions& options);

    virtual ~MountContext();

    /** Returns the FUSE mount path */
    const std::string& GetMountPath() const;

private:

    /** True if the mount point is home-relative */
    bool mHomeRelative { false };

    std::unique_ptr<Andromeda::Filesystem::Folder> mRootFolder;
    std::unique_ptr<AndromedaFuse::FuseAdapter> mFuseAdapter;

    Andromeda::Debug mDebug;
};

} // namespace AndromedaGui

#endif // A2GUI_MOUNTCONTEXT_H
