
#include <iostream>
#include <functional>
#include <bitset>

#if USE_FUSE2
#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#else
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#endif

#include "FuseWrapper.hpp"
#include "HTTPRunner.hpp"
#include "Backend.hpp"
#include "filesystem/Item.hpp"
#include "filesystem/File.hpp"
#include "filesystem/Folder.hpp"

static Debug debug("FuseWrapper");
static Folder* rootPtr = nullptr;

static FuseWrapper::Options const* optionsPtr = nullptr;

static int a2fuse_statfs(const char *path, struct statvfs* buf);
static int a2fuse_access(const char* path, int mask);
static int a2fuse_open(const char* path, struct fuse_file_info* fi);
static int a2fuse_opendir(const char* path, struct fuse_file_info* fi);
static int a2fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi);
static int a2fuse_mkdir(const char* path, mode_t mode);
static int a2fuse_unlink(const char* path);
static int a2fuse_rmdir(const char* path);
static int a2fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi);
static int a2fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi);
static int a2fuse_flush(const char* path, struct fuse_file_info* fi);
static int a2fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi);
static int a2fuse_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
static void a2fuse_destroy(void* private_data);

#if USE_FUSE2
static void* a2fuse_init(struct fuse_conn_info* conn);
static int a2fuse_getattr(const char* path, struct stat* stbuf);
static int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int a2fuse_rename(const char* oldpath, const char* newpath);
static int a2fuse_truncate(const char* path, off_t size);
static int a2fuse_chmod(const char* path, mode_t mode);
static int a2fuse_chown(const char* path, uid_t uid, gid_t gid);
#else
static void* a2fuse_init(struct fuse_conn_info* conn, struct fuse_config* cfg);
static int a2fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
static int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
static int a2fuse_rename(const char* oldpath, const char* newpath, unsigned int flags);
static int a2fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi);
static int a2fuse_chmod(const char* path, mode_t mode, struct fuse_file_info* fi);
static int a2fuse_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi);
#endif

static struct fuse_operations a2fuse_ops = {
    .getattr = a2fuse_getattr,
    .mkdir = a2fuse_mkdir,
    .unlink = a2fuse_unlink,
    .rmdir = a2fuse_rmdir,
    .rename = a2fuse_rename,
    .chmod = a2fuse_chmod,
    .chown = a2fuse_chown,
    .truncate = a2fuse_truncate,
    .open = a2fuse_open,
    .read = a2fuse_read,
    .write = a2fuse_write,
    .statfs = a2fuse_statfs,
    .flush = a2fuse_flush,
    .fsync = a2fuse_fsync,
    .opendir = a2fuse_opendir,
    .readdir = a2fuse_readdir,
    .fsyncdir = a2fuse_fsyncdir,
    .init = a2fuse_init,
    .destroy = a2fuse_destroy,
    .access = a2fuse_access,
    .create = a2fuse_create
};

constexpr int SUCCESS = 0;

/*****************************************************/
void FuseWrapper::ShowVersionText()
{
    std::cout << "libfuse version: " << fuse_version()
#if !USE_FUSE2
        << " (" << fuse_pkgversion() << ")"
#endif
        << std::endl;

#if !USE_FUSE2
    fuse_lowlevel_version();
#endif
}

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseArguments
{
    FuseArguments():args(FUSE_ARGS_INIT(0,NULL))
    {
        if (fuse_opt_add_arg(&args, "andromeda-fuse"))
            throw FuseWrapper::Exception("fuse_opt_add_arg()1 failed");
    };
    ~FuseArguments()
    { 
        debug << __func__ << "() fuse_opt_free_args()"; debug.Info();

        fuse_opt_free_args(&args); 
    };
    /** @param arg -o fuse argument */
    void AddArg(const std::string& arg)
    { 
        debug << __func__ << "(arg:" << arg << ")"; debug.Info();

        if (fuse_opt_add_arg(&args, "-o") != SUCCESS)
            throw FuseWrapper::Exception("fuse_opt_add_arg()2 failed");

        if (fuse_opt_add_arg(&args, arg.c_str()) != SUCCESS)
            throw FuseWrapper::Exception("fuse_opt_add_arg()3 failed");
    };
    /** fuse_args struct */
    struct fuse_args args;
};

#if USE_FUSE2

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param fargs FuseArguments reference
     * @param path filesystem path to mount */
    FuseMount(FuseArguments& fargs, const char* path):path(path)
    {
        debug << __func__ << "() fuse_mount()"; debug.Info();
        
        chan = fuse_mount(path, &fargs.args);
        
        if (!chan) throw FuseWrapper::Exception("fuse_mount() failed");
    };
    void Unmount()
    {
        if (chan == nullptr) return;

        debug << __func__ << "() fuse_unmount()"; debug.Info();
        
        fuse_unmount(path.c_str(), chan); chan = nullptr;
    };
    ~FuseMount(){ Unmount(); };
    
    /** mounted path */
    const std::string path;
    /** fuse_chan pointer */
    struct fuse_chan* chan;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param mount FuseMount reference 
     * @param fargs FuseArguments reference */
    FuseContext(FuseMount& mount, FuseArguments& fargs):mount(mount)
    { 
        debug << __func__ << "() fuse_new()"; debug.Info();

        fuse = fuse_new(mount.chan, &(fargs.args), &a2fuse_ops, sizeof(a2fuse_ops), (void*)nullptr);

        if (!fuse) throw FuseWrapper::Exception("fuse_new() failed");
    };
    ~FuseContext()
    {
        mount.Unmount(); 

        debug << __func__ << "() fuse_destroy()"; debug.Info();

        fuse_destroy(fuse);
    };
    /** Fuse context pointer */
    struct fuse* fuse;
    /** Fuse mount reference */
    FuseMount& mount;
};

#else

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param fargs FuseArguments reference */
    FuseContext(FuseArguments& fargs)
    {
        debug << __func__ << "() fuse_new()"; debug.Info();
        
        fuse = fuse_new(&(fargs.args), &a2fuse_ops, sizeof(a2fuse_ops), (void*)nullptr);
        
        if (!fuse) throw FuseWrapper::Exception("fuse_new() failed");
    };
    ~FuseContext()
    {
        debug << __func__ << "() fuse_destroy()"; debug.Info();
        
        fuse_destroy(fuse);
    };
    /** Fuse context pointer */
    struct fuse* fuse;
};

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param context FuseContext reference
     * @param path filesystem path to mount */
    FuseMount(FuseContext& context, const char* path):fuse(context.fuse)
    {
        debug << __func__ << "() fuse_mount()"; debug.Info();

        if (fuse_mount(fuse, path) != SUCCESS)
            throw FuseWrapper::Exception("fuse_mount() failed");
    };
    ~FuseMount()
    {
        debug << __func__ << "() fuse_unmount()"; debug.Info();
        
        fuse_unmount(fuse);
    }
    /** Fuse context pointer */
    struct fuse* fuse;
};

#endif

/*****************************************************/
/** Scope-managed fuse_set/remove_signal_handlers */
struct FuseSignals
{
    /** @param context FuseContext reference */
    FuseSignals(FuseContext& context)
    { 
        debug << __func__ << "() fuse_set_signal_handlers()"; debug.Info();

        session = fuse_get_session(context.fuse);

        if (fuse_set_signal_handlers(session) != SUCCESS)
            throw FuseWrapper::Exception("fuse_set_signal_handlers() failed");
    };
    ~FuseSignals()
    {
        debug << __func__ << "() fuse_remove_signal_handlers()"; debug.Info();

        fuse_remove_signal_handlers(session); 
    }
    /** fuse_session pointer */
    struct fuse_session* session;
};

/*****************************************************/
/** Scope-managed fuse event loop - blocks */
struct FuseLoop
{
    /** @param context FUSE context reference */
    FuseLoop(FuseContext& context)
    {
        debug << __func__ << "() fuse_loop()"; debug.Info();

        int retval = fuse_loop(context.fuse);

        debug << __func__ << "() fuse_loop() returned! retval:" << retval; debug.Info();
    }
};

/*****************************************************/
void FuseWrapper::Start(Folder& root, const FuseWrapper::Options& options)
{
    rootPtr = &root; optionsPtr = &options;

    debug << __func__ << "(path:" << options.mountPath << ")"; debug.Info();

    FuseArguments opts; for (const std::string& opt : options.fuseArgs) opts.AddArg(opt);

#if USE_FUSE2
    FuseMount mount(opts, options.mountPath.c_str());
    FuseContext context(mount, opts);
#else
    FuseContext context(opts); 
    FuseMount mount(context, options.mountPath.c_str());
#endif
    
    debug << __func__ << "... fuse_daemonize()"; debug.Info();
    if (fuse_daemonize(static_cast<bool>(Debug::GetLevel())) != SUCCESS)
        throw FuseWrapper::Exception("fuse_daemonize() failed");
    
    FuseSignals signals(context); 
    FuseLoop loop(context);
}

/*****************************************************/
/*****************************************************/
/*****************************************************/

/*****************************************************/
#if USE_FUSE2
void* a2fuse_init(struct fuse_conn_info* conn)
#else
void* a2fuse_init(struct fuse_conn_info* conn, struct fuse_config* cfg)
#endif
{
    debug << __func__ << "()"; debug.Info();

#if !USE_FUSE2
    conn->time_gran = 1000; // PHP microseconds
    cfg->negative_timeout = 1;
#endif

    return NULL;
}

/*****************************************************/
int standardTry(const std::string& fname, std::function<int()> func)
{
    try { return func(); }

    // Item exceptions
    catch (const Folder::NotFileException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTDIR;
    }
    catch (const Folder::NotFoundException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOENT;
    }
    catch (const Folder::DuplicateItemException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EEXIST;
    }
    catch (const Folder::ModifyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const File::WriteTypeException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const Item::ReadOnlyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EROFS;
    }

    // Backend exceptions
    catch (const Backend::UnsupportedException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const Backend::ReadOnlyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EROFS;
    }
    catch (const Backend::DeniedException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EACCES;
    }
    catch (const Backend::NotFoundException& e)  
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOENT;
    }

    // Error exceptions
    catch (const HTTPRunner::ConnectionException& e)
    {
        debug << fname << "..." << e.what(); debug.Error(); return -EHOSTDOWN;
    }
    catch (const Utilities::Exception& e)
    {
        debug << fname << "..." << e.what(); debug.Error(); return -EIO;
    }
}

/*****************************************************/
int a2fuse_statfs(const char *path, struct statvfs* buf)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    buf->f_namemax = 255;
    
    // The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored 
//           struct statvfs {
//           unsigned long  f_bsize;    /* Filesystem block size */
//           unsigned long  f_frsize;   /* Fragment size */
//           fsblkcnt_t     f_blocks;   /* Size of fs in f_frsize units */
//           fsblkcnt_t     f_bfree;    /* Number of free blocks */
//           fsblkcnt_t     f_bavail;   /* Number of free blocks for
//                                         unprivileged users */
//           fsfilcnt_t     f_files;    /* Number of inodes */
//           fsfilcnt_t     f_ffree;    /* Number of free inodes */
//    ///    fsfilcnt_t     f_favail;   /* Number of free inodes for
//                                         unprivileged users */
//    ///    unsigned long  f_fsid;     /* Filesystem ID */
//    ///    unsigned long  f_flag;     /* Mount flags */
//           unsigned long  f_namemax;  /* Maximum filename length */

// files getlimits & files getlimits --filesystem to calculate free space?
// then the root folder knows the total space used...?
// DO actually need the path as the answer would be different for each SuperRoot filesystem
// this could be a call into a filesystem object... superRoot would need to extend Filesystem

    return -EIO; // TODO
}

/*****************************************************/
void item_stat(const Item& item, struct stat* stbuf)
{
    switch (item.GetType())
    {
        case Item::Type::FILE: stbuf->st_mode = S_IFREG; break;
        case Item::Type::FOLDER: stbuf->st_mode = S_IFDIR; break;
    }

    stbuf->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;

    if (item.isReadOnly()) stbuf->st_mode &= ~S_IWUSR & ~S_IWGRP & ~S_IWOTH;

    stbuf->st_size = static_cast<off_t>(item.GetSize());

    stbuf->st_ctime = static_cast<time_t>(item.GetCreated());

    stbuf->st_mtime = static_cast<time_t>(item.GetModified());
    if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;

    stbuf->st_atime = static_cast<time_t>(item.GetAccessed());
    if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_ctime;
}

/*****************************************************/
int a2fuse_access(const char* path, int mask)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(rootPtr->GetItemByPath(path));

        if (mask & W_OK && item.isReadOnly()) return -EROFS;

        return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_open(const char* path, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(rootPtr->GetItemByPath(path));

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR)
            && item.isReadOnly()) return -EROFS;

        return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_opendir(const char* path, struct fuse_file_info* fi)
{
    return a2fuse_open(path, fi);
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_getattr(const char* path, struct stat* stbuf)
#else
int a2fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        item_stat(rootPtr->GetItemByPath(path), stbuf); return SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
#else
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        const Folder::ItemMap& items(rootPtr->GetFolderByPath(path).GetItems());

        debug << __func__ << "... #items:" << items.size(); debug.Info();

        for (const Folder::ItemMap::value_type& pair : items)
        {
            const std::unique_ptr<Item>& item = pair.second;

            debug << __func__ << "... subitem: " << item->GetName(); debug.Info();
            
#if USE_FUSE2
            int retval = filler(buf, item->GetName().c_str(), NULL, 0);
#else
            int retval; if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf; item_stat(*item, &stbuf);

                retval = filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName().c_str(), NULL, 0, (fuse_fill_dir_flags)0);
#endif
            if (retval != SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        for (const char* name : {".",".."})
        {
#if USE_FUSE2
            int retval = filler(buf, name, NULL, 0);
#else
            int retval = filler(buf, name, NULL, 0, (fuse_fill_dir_flags)0);
#endif
            if (retval != SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_create(const char* fullpath, mode_t mode, struct fuse_file_info* fi)
{
    fullpath++; const Utilities::StringPair pair(Utilities::split(fullpath,"/",true));
    const std::string& path = pair.first; const std::string& name = pair.second;

    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(rootPtr->GetFolderByPath(path));

        parent.CreateFile(name); return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_mkdir(const char* fullpath, mode_t mode)
{
    fullpath++; const Utilities::StringPair pair(Utilities::split(fullpath,"/",true));
    const std::string& path = pair.first; const std::string& name = pair.second;
    
    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(rootPtr->GetFolderByPath(path));

        parent.CreateFolder(name); return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_unlink(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        rootPtr->GetFileByPath(path).Delete(); return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_rmdir(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        rootPtr->GetFolderByPath(path).Delete(); return SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_rename(const char* oldpath, const char* newpath)
#else
int a2fuse_rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif
{
    oldpath++; const Utilities::StringPair pair0(Utilities::split(oldpath,"/",true));
    const std::string& path0 = pair0.first; const std::string& name0 = pair0.second;

    newpath++; const Utilities::StringPair pair1(Utilities::split(newpath,"/",true));
    const std::string& path1 = pair1.first; const std::string& name1 = pair1.second;

    debug << __func__ << "(oldpath:" << oldpath << " newpath:" << newpath << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(rootPtr->GetItemByPath(oldpath));

        if (path0 != path1 && name0 != name1)
        {
            Folder& parent(rootPtr->GetFolderByPath(path1));

            debug << "NOT SUPPORTED YET!"; debug.Error(); return -EIO; // TODO
        }
        else if (path0 != path1)
        {
            Folder& parent(rootPtr->GetFolderByPath(path1));

            item.Move(parent, true);
        }
        else if (name0 != name1) 
        {
            item.Rename(name1, true);
        }

        return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        return file.ReadBytes(reinterpret_cast<std::byte*>(buf), off, size);
    });
}

/*****************************************************/
int a2fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        file.WriteBytes(reinterpret_cast<const std::byte*>(buf), off, size); return size;
    });
}

/*****************************************************/
int a2fuse_flush(const char* path, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        file.FlushCache(); return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        file.FlushCache(); return SUCCESS;
    });
}

/*****************************************************/
int a2fuse_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& folder(rootPtr->GetFolderByPath(path));

        folder.FlushCache(); return SUCCESS;
    });
}

/*****************************************************/
void a2fuse_destroy(void* private_data)
{
    debug << __func__ << "()"; debug.Info();

    standardTry(__func__,[&]()->int
    {
        rootPtr->FlushCache(true); return SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_truncate(const char* path, off_t size)
#else
int a2fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        file.Truncate(size); return SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_chmod(const char* path, mode_t mode)
#else
int a2fuse_chmod(const char* path, mode_t mode, struct fuse_file_info* fi)
#endif
{
    if (!optionsPtr->fakeChmod) return -ENOTSUP;

    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        rootPtr->GetFileByPath(path); return SUCCESS; // no-op
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_chown(const char* path, uid_t uid, gid_t gid)
#else
int a2fuse_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi)
#endif
{
    if (!optionsPtr->fakeChown) return -ENOTSUP;

    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        rootPtr->GetFileByPath(path); return SUCCESS; // no-op
    });
}
