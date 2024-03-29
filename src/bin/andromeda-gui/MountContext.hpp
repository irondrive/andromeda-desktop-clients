#ifndef A2GUI_MOUNTCONTEXT_H
#define A2GUI_MOUNTCONTEXT_H

#include <filesystem>
#include <memory>
#include <string>

#include "andromeda/common.hpp"
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

    /** Exception indicating the desired mount directory is not found */
    class MountNotFoundException : public Exception { public:
        explicit MountNotFoundException(const std::string& path) : 
            Exception("Mount Directory not found:\n\n"+path) {}; };

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
     * @param autoHome if true, mountPath is $HOME-relative and will be created, else path is absolute
     * @param mountPath filesystem path to mount - must already exist if not autoHome
     * @param options FUSE adapter options
     * @throws Exception if there is an error creating the mountpoint
     * @throws FuseAdapter::Exception if there is a FUSE error
     */
    MountContext(Andromeda::Backend::BackendImpl& backend,
        bool autoHome, std::string mountPath, 
        AndromedaFuse::FuseOptions& options);

    virtual ~MountContext();
    DELETE_COPY(MountContext)
    DELETE_MOVE(MountContext)

    /** Returns the FUSE mount path */
    const std::string& GetMountPath() const;

private:

    /** True if the mount point is auto created */
    bool mCreateMount { false };

    std::unique_ptr<Andromeda::Filesystem::Folder> mRootFolder;
    std::unique_ptr<AndromedaFuse::FuseAdapter> mFuseAdapter;

    mutable Andromeda::Debug mDebug;
};

} // namespace AndromedaGui

#endif // A2GUI_MOUNTCONTEXT_H
