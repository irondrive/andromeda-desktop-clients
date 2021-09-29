
#ifndef LIBA2_FILESYSTEMS_H_
#define LIBA2_FILESYSTEMS_H_

#include "../Folder.hpp"

/** A folder that lists filesystems */
class Filesystems : public Folder
{
public:

    Filesystems(Backend& backend, Folder& parent);
    
    virtual ~Filesystems(){};

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubDeleteItem(Item& item) override;

    virtual void SubRenameItem(Item& item, const std::string& name, bool overwrite) override;

    virtual void SubMoveItem(Item& item, Folder& parent, bool overwrite) override { throw ModifyException(); }
    
    virtual bool CanReceiveItems() override { return false; }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& name, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& parent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
