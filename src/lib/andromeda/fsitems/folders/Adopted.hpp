
#ifndef LIBA2_ADOPTED_H_
#define LIBA2_ADOPTED_H_

#include "PlainFolder.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
class Backend;

namespace FSItems {
class Folder;

namespace Folders {

/** A special folder listing items owned but residing in other users' folders */
class Adopted : public PlainFolder
{
public:

    virtual ~Adopted(){};

    /**
     * @param backend backend reference
     * @param parent reference to parent 
     */
    Adopted(Backend& backend, Folder& parent);

protected:

    /** populate itemMap from the backend */
    virtual void LoadItems() override;
        
    virtual bool isReadOnly() const override { return true; }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& newName, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& newParent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug mDebug;
};

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders

#endif
