#ifndef LIBA2_PLAINFOLDER_H_
#define LIBA2_PLAINFOLDER_H_

#include <string>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "../Folder.hpp"
#include "Utilities.hpp"

class Backend;

/** A regular Andromeda folder */
class PlainFolder : public Folder
{
public:

    virtual ~PlainFolder(){};

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    static std::unique_ptr<PlainFolder> LoadByID(Backend& backend, const std::string& id);
    
    /** 
     * Construct with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     * @param parent pointer to parent
     * @param haveItems true if JSON has subitems
     */
    PlainFolder(Backend& backend, const nlohmann::json* data = nullptr, Folder* parent = nullptr, bool haveItems = false);

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override;

    virtual void SubCreateFolder(const std::string& name) override;
    
    virtual void SubDeleteItem(Item& item) override;

    virtual void SubRenameItem(Item& item, const std::string& name, bool overwrite) override;

    virtual void SubMoveItem(Item& item, Folder& parent, bool overwrite) override;

    virtual void SubDelete() override;

    virtual void SubRename(const std::string& name, bool overwrite) override;

    virtual void SubMove(Folder& parent, bool overwrite) override;

private:

    Debug debug;
};

#endif
